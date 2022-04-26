#include <detail/shared_impl.hpp>
#include <vulkify/graphics/buffer.hpp>

namespace vf {
Buffer::Buffer() noexcept = default;
Buffer::Buffer(Buffer&&) noexcept = default;
Buffer& Buffer::operator=(Buffer&&) noexcept = default;
Buffer::~Buffer() noexcept = default;

Buffer::Buffer(Vram const& vram, std::string name) : m_name(std::move(name)) { m_resource->vram = vram; }

Buffer::operator bool() const { return static_cast<bool>(vram()); }

Vram const& Buffer::vram() const { return m_resource->vram; }

GeometryBuffer::GeometryBuffer(Vram const& vram, std::string name) : Buffer(vram, std::move(name)) {}

static std::size_t vboSize(Geometry const& geometry) { return geometry.vertices.size() * sizeof(decltype(geometry.vertices[0])); }
static std::size_t iboSize(Geometry const& geometry) { return geometry.indices.size() * sizeof(decltype(geometry.indices[0])); }

bool GeometryBuffer::write(Geometry geometry) {
	if (!m_resource->vram) { return false; }
	auto& buffers = m_resource->buffer.buffers;
	bool const vboSpace = !buffers.empty() && buffers[0]->size >= vboSize(geometry);
	bool const iboSpace = geometry.indices.empty() || (buffers.size() > 1 && buffers[1]->size >= iboSize(geometry));
	if (!vboSpace || !iboSpace) {
		m_resource->vram.device.defer(std::move(m_resource->buffer));
		m_resource->buffer = m_resource->vram.makeVIBuffer(geometry, BufferObject::Type::eCpuToGpu, m_name.c_str());
	} else {
		buffers[0]->write(geometry.vertices.data(), geometry.vertices.size());
		if (buffers.size() > 1) { buffers[1]->write(geometry.indices.data(), geometry.indices.size()); }
	}
	m_geometry = std::move(geometry);
	return true;
}

UniformBuffer::UniformBuffer(Vram const& vram, std::string name) : Buffer(vram, std::move(name)) {}

std::size_t UniformBuffer::size() const { return m_resource->buffer.buffers.empty() ? 0 : m_resource->buffer.buffers[0]->size; }

bool UniformBuffer::resize(std::size_t size) {
	if (!m_resource->vram) { return false; }
	auto buffer = m_resource->vram.makeBuffer({{}, size, vk::BufferUsageFlagBits::eUniformBuffer}, VMA_MEMORY_USAGE_CPU_TO_GPU, m_name.c_str());
	if (!buffer) { return false; }
	auto& buffers = m_resource->buffer.buffers;
	if (buffers.empty()) {
		buffers.push_back({});
	} else {
		m_resource->vram.device.defer(std::move(buffers.front()));
	}
	buffers.front() = std::move(buffer);
	return true;
}

bool UniformBuffer::write(void const* data, std::size_t size) {
	if (size == 0 || !m_resource->vram) { return false; }
	if (this->size() < size && !resize(size)) { return false; }
	return m_resource->buffer.buffers[0]->write(data, size);
}
} // namespace vf
