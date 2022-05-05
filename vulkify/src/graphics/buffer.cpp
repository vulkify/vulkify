#include <detail/shared_impl.hpp>
#include <vulkify/context/context.hpp>
#include <vulkify/graphics/buffer.hpp>

namespace vf {
GeometryBuffer::GeometryBuffer(Context const& context, std::string name) : GfxResource(context.vram(), std::move(name)) {}

Result<void> GeometryBuffer::write(Geometry geometry) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }
	if (geometry.vertices.empty()) { return Error::eInvalidArgument; }

	m_allocation->vram.device.defer(std::move(m_allocation->buffer));
	m_allocation->buffer = m_allocation->vram.makeVIBuffer(geometry, BufferCache::Type::eCpuToGpu, m_allocation->name.c_str());

	m_geometry = std::move(geometry);
	return Result<void>::success();
}

UniformBuffer::UniformBuffer(Context const& context, std::string name) : GfxResource(context.vram(), std::move(name)) {}

std::size_t UniformBuffer::size() const { return m_allocation->buffer.buffers.empty() ? 0 : m_allocation->buffer.buffers[0]->size; }

Result<void> UniformBuffer::resize(std::size_t const size) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }
	if (size == 0) { return Error::eInvalidArgument; }

	auto buffer = m_allocation->vram.makeBuffer({{}, size, vk::BufferUsageFlagBits::eUniformBuffer}, true, m_allocation->name.c_str());
	if (!buffer) { return Error::eMemoryError; }
	auto& buffers = m_allocation->buffer.buffers;
	if (buffers.empty()) {
		buffers.push_back({});
	} else {
		m_allocation->vram.device.defer(std::move(buffers.front()));
	}

	buffers.front() = std::move(buffer);
	return Result<void>::success();
}

Result<void> UniformBuffer::write(BufferWrite const data) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }
	if (!data.data || data.size == 0) { return Error::eInvalidArgument; }

	if (this->size() < data.size && !resize(data.size)) { return Error::eMemoryError; }
	if (!m_allocation->buffer.buffers[0]->write(data.data, data.size)) { return Error::eMemoryError; }

	return Result<void>::success();
}
} // namespace vf
