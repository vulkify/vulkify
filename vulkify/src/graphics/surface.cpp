#include <detail/pipeline_factory.hpp>
#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/pipeline.hpp>
#include <vulkify/graphics/texture.hpp>
#include <vulkify/instance/instance.hpp>

namespace vf {
bool RenderPass::writeSetOne(std::span<DrawModel const> instances, Tex tex, char const* name) const {
	if (instances.empty() || !tex.sampler || !tex.view) { return false; }
	auto const& sb = shaderInput.one;
	auto set = descriptorPool->postInc(sb.set, name);
	set.write(sb.bindings.ssbo, instances.data(), instances.size() * sizeof(decltype(instances[0])));
	set.update(sb.bindings.sampler, tex.sampler, tex.view);
	set.bind(commandBuffer, bound);
	return true;
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

bool Surface::bind(Pipeline const& pipeline) const {
	if (!m_renderPass->pipelineFactory || !m_renderPass->renderPass) { return false; }
	auto const [pipe, layout] = m_renderPass->pipelineFactory->pipeline({pipeline}, m_renderPass->renderPass);
	if (!pipe || !layout) { return false; }
	if (layout == m_renderPass->bound) { return true; }
	m_renderPass->commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipe);
	m_renderPass->shaderInput.setZero.bind(m_renderPass->commandBuffer, layout);
	m_renderPass->bound = layout;
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
	auto tex = RenderPass::Tex{*m_renderPass->shaderInput.textures->sampler, *m_renderPass->shaderInput.textures->white.view};
	if (drawable.texture) { tex = {*drawable.texture.resource().image.sampler, *drawable.texture.resource().image.cache.view}; }
	auto const& buffers = drawable.gbo.resource().buffer.buffers;
	if (!m_renderPass->writeSetOne(models, tex, drawable.gbo.name().c_str())) { return false; }
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
