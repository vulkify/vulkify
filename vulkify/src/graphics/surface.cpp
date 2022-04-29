#include <detail/pipeline_factory.hpp>
#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/pipeline.hpp>
#include <vulkify/instance/instance.hpp>

namespace vf {
void RenderPass::writeModels(std::span<DrawModel const> instances, char const* name) const {
	if (instances.empty()) { return; }
	auto const binding = shaderInput.model.binding;
	auto set = descriptorPool->postInc(shaderInput.model.set, name);
	set.write(binding, instances.data(), instances.size() * sizeof(decltype(instances[0])));
	set.bind(commandBuffer, bound);
}

Surface::Surface() noexcept = default;
Surface::Surface(RenderPass renderPass) : m_renderPass(std::move(renderPass)) { bind({}); }
Surface::Surface(Surface&& rhs) noexcept : Surface() { std::swap(m_renderPass, rhs.m_renderPass); }
Surface& Surface::operator=(Surface rhs) noexcept { return (std::swap(m_renderPass, rhs.m_renderPass), *this); }

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

bool Surface::draw(GeometryBuffer const& geometry, char const* name, std::span<DrawModel const> models) const {
	if (!m_renderPass->descriptorPool || !m_renderPass->bound) { return false; }
	m_renderPass->writeModels(models, name);
	auto const& buffers = geometry.resource().buffer.buffers;
	if (buffers.empty() || !m_renderPass->commandBuffer) { return false; }
	m_renderPass->commandBuffer.bindVertexBuffers(0, buffers[0]->resource, vk::DeviceSize{});
	auto const& geo = geometry.geometry();
	if (buffers.size() > 1) {
		m_renderPass->commandBuffer.bindIndexBuffer(buffers[1]->resource, vk::DeviceSize{}, vk::IndexType::eUint32);
		m_renderPass->commandBuffer.drawIndexed(static_cast<std::uint32_t>(geo.indices.size()), static_cast<std::uint32_t>(models.size()), 0, 0, 0);
	} else {
		m_renderPass->commandBuffer.draw(static_cast<std::uint32_t>(geo.vertices.size()), static_cast<std::uint32_t>(models.size()), 0, 0);
	}
	return true;
}
} // namespace vf
