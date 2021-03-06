#include <detail/descriptor_set_factory.hpp>
#include <detail/pipeline_factory.hpp>
#include <detail/render_pass.hpp>
#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/resources/geometry_buffer.hpp>
#include <vulkify/graphics/resources/texture.hpp>
#include <vulkify/graphics/shader.hpp>
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

[[maybe_unused]] constexpr auto name_v = "vf::(internal)";
} // namespace

HTexture RenderPass::whiteTexture() const { return HTexture{*shaderInput.textures->white.view, *shaderInput.textures->sampler}; }

void RenderPass::writeView(SetWriter& set) const {
	if (!set) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to write view set");
		return;
	}
	auto const scale = cam.camera->view.getScale(cam.extent);
	// invert transformation
	auto const dm = DrawModel{{-cam.camera->position, cam.camera->orientation.inverted().value()}, {scale, glm::vec2()}};
	set.write(shaderInput.one.bindings.ubo, &dm, sizeof(dm));
}

void RenderPass::writeModels(SetWriter& set, std::span<DrawModel const> instances, TextureHandle const& texture) const {
	auto const tex = texture.handle.contains<HTexture>() ? texture.handle.get<HTexture>() : whiteTexture();
	if (!set || instances.empty() || !tex.sampler || !tex.view) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to write models set");
		return;
	}
	auto const& sb = shaderInput.one;
	set.write(sb.bindings.ssbo, instances.data(), instances.size_bytes());
	set.update(sb.bindings.sampler, tex.sampler, tex.view);
	set.bind(commandBuffer, bound);
}

void RenderPass::writeCustom(SetWriter& set, std::span<std::byte const> ubo, TextureHandle const& texture) const {
	auto const tex = texture.handle.contains<HTexture>() ? texture.handle.get<HTexture>() : whiteTexture();
	if (!set || !tex.sampler || !tex.view) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to write custom set");
		return;
	}
	static constexpr auto byte_v = std::byte{};
	if (ubo.empty()) { ubo = {&byte_v, 1}; }
	auto const& sb = shaderInput.two;
	set.write(sb.bindings.ubo, ubo.data(), ubo.size_bytes());
	set.update(sb.bindings.sampler, tex.sampler, tex.view);
	set.bind(commandBuffer, bound);
}

void RenderPass::bind(vk::PipelineLayout layout, vk::Pipeline pipeline) const {
	if (layout == bound) { return; }
	if (!layout || !commandBuffer) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to bind pipeline");
		return;
	}
	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
	shaderInput.mat_p.bind(commandBuffer, layout);
	bound = layout;
}

void RenderPass::setViewport() const {
	if (!commandBuffer || !cam.camera) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to set viewport");
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

bool Surface::draw(Drawable const& drawable, RenderState const& state) const {
	if (!m_renderPass->pipelineFactory || !m_renderPass->renderPass) { return false; }
	if (drawable.instances.empty() || !drawable.gbo) { return false; }
	if (drawable.instances.size() <= small_buffer_v) {
		auto buffer = ktl::fixed_vector<DrawModel, small_buffer_v>{};
		addDrawModels(drawable.instances, std::back_inserter(buffer));
		return draw(buffer, drawable, state);
	} else {
		auto buffer = std::vector<DrawModel>{};
		buffer.reserve(drawable.instances.size());
		addDrawModels(drawable.instances, std::back_inserter(buffer));
		return draw(buffer, drawable, state);
	}
}

bool Surface::bind(RenderState const& state) const {
	auto program = PipelineFactory::Spec::ShaderProgram{};
	if (state.descriptorSet) { program.frag = *state.descriptorSet->m_shader->m_impl->module; }
	auto const spec = PipelineFactory::Spec{program, polygonMode(state.pipeline.polygonMode), topology(state.pipeline.topology)};
	auto const [pipe, layout] = m_renderPass->pipelineFactory->pipeline(spec, m_renderPass->renderPass);
	if (!pipe || !layout) { return false; }
	m_renderPass->bind(layout, pipe);
	return true;
}

bool Surface::draw(std::span<DrawModel const> models, Drawable const& drawable, RenderState const& state) const {
	if (!m_renderPass->pipelineFactory || !m_renderPass->renderPass || !m_renderPass->renderMutex) { return false; }
	auto lock = std::scoped_lock(*m_renderPass->renderMutex);
	if (!bind(state)) { return false; }

	auto set = m_renderPass->setFactory->postInc(m_renderPass->shaderInput.one.set, "UBO:V,SSBO:M");
	if (!set) { return false; }
	m_renderPass->writeView(set);
	m_renderPass->writeModels(set, models, drawable.texture);
	if (state.descriptorSet) {
		set = m_renderPass->setFactory->postInc(m_renderPass->shaderInput.two.set, "Custom");
		if (!set) { return false; }
		m_renderPass->writeCustom(set, state.descriptorSet->m_data.bytes, state.descriptorSet->m_data.texture);
	}
	m_renderPass->setViewport();
	auto const lineWidth = std::clamp(state.pipeline.lineWidth, m_renderPass->lineWidthLimit.first, m_renderPass->lineWidthLimit.second);
	m_renderPass->commandBuffer.setLineWidth(lineWidth);

	auto const& vbo = drawable.gbo.resource().buffers[0].get(true);
	m_renderPass->commandBuffer.bindVertexBuffers(0, vbo.resource, vk::DeviceSize{});
	auto const counts = drawable.gbo.counts();
	auto const instanceCount = static_cast<std::uint32_t>(models.size());
	if (counts.indices > 0) {
		auto const& ibo = drawable.gbo.resource().buffers[1].get(true);
		m_renderPass->commandBuffer.bindIndexBuffer(ibo.resource, vk::DeviceSize{}, vk::IndexType::eUint32);
		m_renderPass->commandBuffer.drawIndexed(counts.indices, instanceCount, 0, 0, 0);
	} else {
		m_renderPass->commandBuffer.draw(counts.vertices, instanceCount, 0, 0);
	}
	return true;
}
} // namespace vf
