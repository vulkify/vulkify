#include <detail/shared_impl.hpp>
#include <vulkify/graphics/buffer.hpp>

namespace vf {
static std::size_t vboSize(Geometry const& geometry) { return geometry.vertices.size() * sizeof(decltype(geometry.vertices[0])); }
static std::size_t iboSize(Geometry const& geometry) { return geometry.indices.size() * sizeof(decltype(geometry.indices[0])); }

Result<void> GeometryBuffer::write(Geometry geometry) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }

	auto& buffers = m_allocation->buffer.buffers;
	bool const vboSpace = !buffers.empty() && buffers[0]->size >= vboSize(geometry);
	bool const iboSpace = geometry.indices.empty() || (buffers.size() > 1 && buffers[1]->size >= iboSize(geometry));
	if (!vboSpace || !iboSpace) {
		m_allocation->vram.device.defer(std::move(m_allocation->buffer));
		m_allocation->buffer = m_allocation->vram.makeVIBuffer(geometry, BufferCache::Type::eCpuToGpu, m_allocation->name.c_str());
	} else {
		buffers[0]->write(geometry.vertices.data(), geometry.vertices.size());
		if (buffers.size() > 1) { buffers[1]->write(geometry.indices.data(), geometry.indices.size()); }
	}

	m_geometry = std::move(geometry);
	return Result<void>::success();
}

std::size_t UniformBuffer::size() const { return m_allocation->buffer.buffers.empty() ? 0 : m_allocation->buffer.buffers[0]->size; }

Result<void> UniformBuffer::resize(std::size_t const size) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }

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

	if (data.size == 0) { return Error::eInvalidArgument; }
	if (this->size() < data.size && !resize(data.size)) { return Error::eInvalidArgument; }
	if (!m_allocation->buffer.buffers[0]->write(data.data, data.size)) { return Error::eMemoryError; }

	return Result<void>::success();
}
} // namespace vf
