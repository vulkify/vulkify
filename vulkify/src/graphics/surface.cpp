#include <detail/pipeline_factory.hpp>
#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/draw_params.hpp>
#include <vulkify/graphics/pipeline_state.hpp>
#include <vulkify/instance/instance.hpp>

namespace vf {
void RenderPass::writeDrawParams(DrawParams const& params) const {
	auto set = descriptorPool->postInc(mat.modelMat.set, params.name);
	set.write(mat.modelMat.binding, ubo::Model{params.modelMatrix, params.tint.normalize()});
	set.bind(commandBuffer, bound);
}

Surface::Surface() noexcept = default;
Surface::Surface(RenderPass renderPass) : m_renderPass(std::move(renderPass)) {}
Surface::Surface(Surface&& rhs) noexcept : Surface() { std::swap(m_renderPass, rhs.m_renderPass); }
Surface& Surface::operator=(Surface rhs) noexcept { return (std::swap(m_renderPass, rhs.m_renderPass), *this); }

Surface::~Surface() {
	if (m_renderPass->instance) { m_renderPass->instance.value->endPass(); }
}

Surface::operator bool() const { return m_renderPass->commandBuffer; }

void Surface::setClear(Rgba rgba) const {
	if (m_renderPass->clear) { *m_renderPass->clear = rgba; }
}

bool Surface::bind(PipelineState const& pipeline) const {
	if (!m_renderPass->pipelineFactory || !m_renderPass->renderPass) { return false; }
	auto const [pipe, layout] = m_renderPass->pipelineFactory->pipeline({pipeline}, m_renderPass->renderPass);
	if (!pipe) { return false; }
	m_renderPass->commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipe);
	m_renderPass->mat.setZero.bind(m_renderPass->commandBuffer, layout);
	m_renderPass->bound = layout;
	return true;
}

bool Surface::draw(GeometryBuffer const& geometry, DrawParams const& params) const {
	if (!m_renderPass->descriptorPool || !m_renderPass->bound) { return false; }
	m_renderPass->writeDrawParams(params);
	auto const& buffers = geometry.resource().buffer.buffers;
	if (buffers.empty() || !m_renderPass->commandBuffer) { return false; }
	m_renderPass->commandBuffer.bindVertexBuffers(0, buffers[0]->resource, vk::DeviceSize{});
	auto const& geo = geometry.geometry();
	if (buffers.size() > 1) {
		m_renderPass->commandBuffer.bindIndexBuffer(buffers[1]->resource, vk::DeviceSize{}, vk::IndexType::eUint32);
		m_renderPass->commandBuffer.drawIndexed(static_cast<std::uint32_t>(geo.indices.size()), 1, 0, 0, 0);
	} else {
		m_renderPass->commandBuffer.draw(static_cast<std::uint32_t>(geo.vertices.size()), 1, 0, 0);
	}
	return true;
}
} // namespace vf
