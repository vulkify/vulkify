#include <detail/pipeline_factory.hpp>
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
	if (!set) { return; }
	auto instance = DrawInstance{view.view->transform};
	instance.transform.position = -instance.transform.position;
	set.write(shaderInput.one.bindings.ubo, instance.drawModel());
}

void RenderPass::writeModels(DescriptorSet& set, std::span<DrawModel const> instances, Tex tex) const {
	if (!set || instances.empty() || !tex.sampler || !tex.view) { return; }
	auto const& sb = shaderInput.one;
	set.write(sb.bindings.ssbo, instances.data(), instances.size() * sizeof(decltype(instances[0])));
	set.update(sb.bindings.sampler, tex.sampler, tex.view);
	set.bind(commandBuffer, bound);
}

void RenderPass::bind(vk::PipelineLayout layout, vk::Pipeline pipeline) const {
	if (!layout || layout == bound || !commandBuffer) { return; }
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	shaderInput.mat_p.bind(commandBuffer, layout);
	bound = layout;
}

void RenderPass::setViewport() const {
	if (!commandBuffer || !view.view) { return; }
	auto const ext = glm::vec2(this->view.extent);
	auto const viewport = view.view->viewport;
	auto const v = Rect{viewport.extent * ext, viewport.origin * ext};
	commandBuffer.setViewport(0, vk::Viewport(v.origin.x, v.extent.y + v.origin.y, v.extent.x, -v.extent.y));
}

Surface::Surface() noexcept = default;
Surface::Surface(RenderPass renderPass) : m_renderPass(std::move(renderPass)) { bind({}); }
Surface::Surface(Surface&& rhs) noexcept : Surface() { std::swap(m_renderPass, rhs.m_renderPass); }
Surface& Surface::operator=(Surface&& rhs) noexcept = default;

Surface::~Surface() {
	if (m_renderPass->instance) { m_renderPass->instance.value->endPass(); }
}

Surface::operator bool() const { return m_renderPass->commandBuffer; }

void Surface::setClear(Rgba rgba) const {
	if (m_renderPass->clear) { *m_renderPass->clear = rgba; }
}

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
	if (drawable.instances.empty() || !drawable.gbo || drawable.gbo.resource().buffer.buffers.empty()) { return false; }
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
	bind(drawable.gbo.state);
	auto tex = RenderPass::Tex{*m_renderPass->shaderInput.textures->sampler, *m_renderPass->shaderInput.textures->white.view};
	if (drawable.texture) { tex = {*drawable.texture.resource().image.sampler, *drawable.texture.resource().image.cache.view}; }
	auto const& buffers = drawable.gbo.resource().buffer.buffers;

	auto set = m_renderPass->descriptorPool->postInc(m_renderPass->shaderInput.one.set, "UBO:M,SSBO:V");
	if (!set) { return false; }
	m_renderPass->writeView(set);
	m_renderPass->writeModels(set, models, tex);
	m_renderPass->setViewport();
	m_renderPass->commandBuffer.setLineWidth(drawable.gbo.state.lineWidth);
	m_renderPass->commandBuffer.bindVertexBuffers(0, buffers[0]->resource, vk::DeviceSize{});
	auto const& geo = drawable.gbo.geometry();
	auto const instanceCount = static_cast<std::uint32_t>(models.size());
	if (buffers.size() > 1) {
		m_renderPass->commandBuffer.bindIndexBuffer(buffers[1]->resource, vk::DeviceSize{}, vk::IndexType::eUint32);
		m_renderPass->commandBuffer.drawIndexed(static_cast<std::uint32_t>(geo.indices.size()), instanceCount, 0, 0, 0);
	} else {
		m_renderPass->commandBuffer.draw(static_cast<std::uint32_t>(geo.vertices.size()), instanceCount, 0, 0);
	}
	return true;
}
} // namespace vf
