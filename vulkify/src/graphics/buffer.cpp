#include <detail/shared_impl.hpp>
#include <vulkify/graphics/buffer.hpp>

namespace vf {
static std::size_t vboSize(Geometry const& geometry) { return geometry.vertices.size() * sizeof(decltype(geometry.vertices[0])); }
static std::size_t iboSize(Geometry const& geometry) { return geometry.indices.size() * sizeof(decltype(geometry.indices[0])); }

bool GeometryBuffer::write(Geometry geometry) {
	if (!m_allocation->vram) { return false; }
	auto& buffers = m_allocation->buffer.buffers;
	bool const vboSpace = !buffers.empty() && buffers[0]->size >= vboSize(geometry);
	bool const iboSpace = geometry.indices.empty() || (buffers.size() > 1 && buffers[1]->size >= iboSize(geometry));
	if (!vboSpace || !iboSpace) {
		m_allocation->vram.device.defer(std::move(m_allocation->buffer));
		m_allocation->buffer = m_allocation->vram.makeVIBuffer(geometry, BufferCache::Type::eCpuToGpu, m_name.c_str());
	} else {
		buffers[0]->write(geometry.vertices.data(), geometry.vertices.size());
		if (buffers.size() > 1) { buffers[1]->write(geometry.indices.data(), geometry.indices.size()); }
	}
	m_geometry = std::move(geometry);
	return true;
}

std::size_t UniformBuffer::size() const { return m_allocation->buffer.buffers.empty() ? 0 : m_allocation->buffer.buffers[0]->size; }

bool UniformBuffer::resize(std::size_t const size) {
	if (!m_allocation->vram) { return false; }
	auto buffer = m_allocation->vram.makeBuffer({{}, size, vk::BufferUsageFlagBits::eUniformBuffer}, true, m_name.c_str());
	if (!buffer) { return false; }
	auto& buffers = m_allocation->buffer.buffers;
	if (buffers.empty()) {
		buffers.push_back({});
	} else {
		m_allocation->vram.device.defer(std::move(buffers.front()));
	}
	buffers.front() = std::move(buffer);
	return true;
}

bool UniformBuffer::write(BufferWrite const data) {
	if (data.size == 0 || !m_allocation->vram) { return false; }
	if (this->size() < data.size && !resize(data.size)) { return false; }
	return m_allocation->buffer.buffers[0]->write(data.data, data.size);
}
} // namespace vf
