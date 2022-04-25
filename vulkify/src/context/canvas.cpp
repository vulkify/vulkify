#include <detail/pipeline_factory.hpp>
#include <detail/shared_impl.hpp>
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
	auto const pipe = m_impl->pipelineFactory->get(pipeline, m_impl->renderPass);
	if (!pipe) { return false; }
	m_impl->commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipe);
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
