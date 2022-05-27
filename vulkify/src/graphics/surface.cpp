#include <detail/descriptor_set_factory.hpp>
#include <detail/pipeline_factory.hpp>
#include <detail/render_pass.hpp>
#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/shader.hpp>
#include <vulkify/graphics/texture.hpp>
#include <vulkify/instance/instance.hpp>

namespace vf {
namespace {
constexpr vk::PolygonMode polygonMode(PolygonMode mode) {
	switch (mode) {
	case PolygonMode::ePoint: return vk::PolygonMode::ePoint;
	case PolygonMode::eLine: return vk::PolygonMode::eLine;
	case PolygonMode::eFill:
	default: return vk::PolygonMode::eFill;
	}
}

constexpr vk::PrimitiveTopology topology(Topology topo) {
	switch (topo) {
	case Topology::eLineStrip: return vk::PrimitiveTopology::eLineStrip;
	case Topology::eLineList: return vk::PrimitiveTopology::eLineList;
	case Topology::ePointList: return vk::PrimitiveTopology::ePointList;
	case Topology::eTriangleList: return vk::PrimitiveTopology::eTriangleList;
	case Topology::eTriangleStrip:
	default: return vk::PrimitiveTopology::eTriangleStrip;
	}
}
} // namespace

void RenderPass::writeView(DescriptorSet& set) const {
	if (!set) {
		VF_TRACE("[vf::(internal)] Failed to write view set");
		return;
	}
	auto const scale = cam.camera->view.getScale(cam.extent);
	// invert transformation
	auto const dm = DrawModel{{-cam.camera->position, cam.camera->orientation.inverted().value()}, {scale, glm::vec2()}};
	set.write(shaderInput.one.bindings.ubo, dm);
}

void RenderPass::writeModels(DescriptorSet& set, std::span<DrawModel const> instances, Tex tex) const {
	if (!set || instances.empty() || !tex.sampler || !tex.view) {
		VF_TRACE("[vf::(internal)] Failed to write models set");
		return;
	}
	auto const& sb = shaderInput.one;
	set.write(sb.bindings.ssbo, instances);
	set.update(sb.bindings.sampler, tex.sampler, tex.view);
	set.bind(commandBuffer, bound);
}

void RenderPass::bind(vk::PipelineLayout layout, vk::Pipeline pipeline) const {
	if (layout == bound) { return; }
	if (!layout || !commandBuffer) {
		VF_TRACE("[vf::(internal)] Failed to bind pipeline");
		return;
	}
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	shaderInput.mat_p.bind(commandBuffer, layout);
	bound = layout;
}

void RenderPass::setViewport() const {
	if (!commandBuffer || !cam.camera) {
		VF_TRACE("[vf::(internal)] Failed to set viewport");
		return;
	}
	auto const vp = Rect{{cam.extent * cam.camera->viewport.extent, cam.extent * cam.camera->viewport.offset}};
	commandBuffer.setViewport(0, vk::Viewport(vp.offset.x, vp.offset.y + vp.extent.y, vp.extent.x, -vp.extent.y)); // flip x / negative y
}

Surface::Surface() noexcept = default;
Surface::Surface(RenderPass renderPass) : m_renderPass(std::move(renderPass)) { bind({}); }
Surface::Surface(Surface&& rhs) noexcept : Surface() { std::swap(m_renderPass, rhs.m_renderPass); }
Surface& Surface::operator=(Surface&& rhs) noexcept = default;

Surface::~Surface() {
	if (m_renderPass->instance) { m_renderPass->instance.value->endPass(); }
}

Surface::operator bool() const { return m_renderPass->instance; }

bool Surface::setShader(Shader const& shader) const {
	if (!m_renderPass->pipelineFactory || !m_renderPass->renderPass) { return false; }
	if (!shader.module().module) { return false; }
	m_renderPass->fragShader = *shader.module().module;
	return true;
}

bool Surface::bind(GeometryBuffer::State const& state) const {
	auto program = PipelineFactory::Spec::ShaderProgram{{}, m_renderPass->fragShader};
	auto const spec = PipelineFactory::Spec{program, polygonMode(state.polygonMode), topology(state.topology)};
	auto const [pipe, layout] = m_renderPass->pipelineFactory->pipeline(spec, m_renderPass->renderPass);
	if (!pipe || !layout) { return false; }
	m_renderPass->bind(layout, pipe);
	return true;
}

bool Surface::draw(Drawable const& drawable) const {
	if (!m_renderPass->pipelineFactory || !m_renderPass->renderPass) { return false; }
	if (drawable.instances.empty() || !drawable.gbo || drawable.gbo.resource().buffers[0].data.empty()) { return false; }
	if (drawable.instances.size() <= small_buffer_v) {
		auto buffer = ktl::fixed_vector<DrawModel, small_buffer_v>{};
		addDrawModels(drawable.instances, std::back_inserter(buffer));
		return draw(buffer, drawable);
	} else {
		auto buffer = std::vector<DrawModel>{};
		buffer.reserve(drawable.instances.size());
		addDrawModels(drawable.instances, std::back_inserter(buffer));
		return draw(buffer, drawable);
	}
}

bool Surface::draw(std::span<DrawModel const> models, Drawable const& drawable) const {
	if (!m_renderPass->pipelineFactory || !m_renderPass->renderPass || !m_renderPass->renderMutex) { return false; }
	auto lock = std::scoped_lock(*m_renderPass->renderMutex);
	if (!bind(drawable.gbo.state)) { return false; }

	auto tex = RenderPass::Tex{*m_renderPass->shaderInput.textures->sampler, *m_renderPass->shaderInput.textures->white.view};
	if (drawable.texture && *drawable.texture) { tex = {*drawable.texture->resource().image.sampler, *drawable.texture->resource().image.cache.view}; }

	auto set = m_renderPass->setFactory->postInc(m_renderPass->shaderInput.one.set, "UBO:V,SSBO:M");
	if (!set) { return false; }
	m_renderPass->writeView(set);
	m_renderPass->writeModels(set, models, tex);
	m_renderPass->setViewport();
	auto const lineWidth = std::clamp(drawable.gbo.state.lineWidth, m_renderPass->lineWidthLimit.first, m_renderPass->lineWidthLimit.second);
	m_renderPass->commandBuffer.setLineWidth(lineWidth);

	auto const& vbo = drawable.gbo.resource().buffers[0].get(true);
	m_renderPass->commandBuffer.bindVertexBuffers(0, vbo.resource, vk::DeviceSize{});
	auto const& geo = drawable.gbo.geometry();
	auto const instanceCount = static_cast<std::uint32_t>(models.size());
	if (!geo.indices.empty()) {
		auto const& ibo = drawable.gbo.resource().buffers[1].get(true);
		m_renderPass->commandBuffer.bindIndexBuffer(ibo.resource, vk::DeviceSize{}, vk::IndexType::eUint32);
		m_renderPass->commandBuffer.drawIndexed(static_cast<std::uint32_t>(geo.indices.size()), instanceCount, 0, 0, 0);
	} else {
		m_renderPass->commandBuffer.draw(static_cast<std::uint32_t>(geo.vertices.size()), instanceCount, 0, 0);
	}
	return true;
}
} // namespace vf
