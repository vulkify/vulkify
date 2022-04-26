#include <detail/pipeline_factory.hpp>
#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/instance/instance.hpp>

namespace vf {
Canvas::Canvas() noexcept = default;
Canvas::Canvas(Impl impl) : m_impl(std::move(impl)) {}
Canvas::Canvas(Canvas&& rhs) noexcept : Canvas() { std::swap(m_impl, rhs.m_impl); }
Canvas& Canvas::operator=(Canvas rhs) noexcept { return (std::swap(m_impl, rhs.m_impl), *this); }

Canvas::~Canvas() noexcept {
	if (m_impl->instance) { m_impl->instance.value->endPass(); }
}

Canvas::operator bool() const { return m_impl->commandBuffer; }

void Canvas::setClear(Rgba rgba) const {
	if (m_impl->clear) { *m_impl->clear = rgba; }
}

bool Canvas::bind(PipelineSpec const& pipeline) const {
	if (!m_impl->pipelineFactory || !m_impl->renderPass) { return false; }
	auto const [pipe, layout] = m_impl->pipelineFactory->pipeline(pipeline, m_impl->renderPass);
	if (!pipe) { return false; }
	m_impl->commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipe);
	m_impl->commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, m_impl->mat.number, m_impl->mat.set, {});
	return true;
}

bool Canvas::draw(GeometryBuffer const& vbo) const {
	auto const& buffers = vbo.resource().buffer.buffers;
	if (buffers.empty() || !m_impl->commandBuffer) { return false; }
	m_impl->commandBuffer.bindVertexBuffers(0, buffers[0]->resource, vk::DeviceSize{});
	auto const& geometry = vbo.geometry();
	if (buffers.size() > 1) {
		m_impl->commandBuffer.bindIndexBuffer(buffers[1]->resource, vk::DeviceSize{}, vk::IndexType::eUint32);
		m_impl->commandBuffer.drawIndexed(static_cast<std::uint32_t>(geometry.indices.size()), 1, 0, 0, 0);
	} else {
		m_impl->commandBuffer.draw(static_cast<std::uint32_t>(geometry.vertices.size()), 1, 0, 0);
	}
	return true;
}

void Canvas::draw(std::size_t vertexCount, std::size_t indexCount) const {
	if (!m_impl->commandBuffer) { return; }
	if (indexCount > 0) {
		m_impl->commandBuffer.drawIndexed(static_cast<std::uint32_t>(indexCount), 1, 0, 0, 0);
	} else {
		m_impl->commandBuffer.draw(static_cast<std::uint32_t>(vertexCount), 1, 0, 0);
	}
}
} // namespace vf
