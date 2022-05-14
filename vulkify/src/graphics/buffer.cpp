#include <detail/shared_impl.hpp>
#include <vulkify/context/context.hpp>
#include <vulkify/graphics/buffer.hpp>

namespace vf {
namespace {
void writeGeometry(BufferCaches& buffers, Geometry const& geometry) {
	assert(!geometry.vertices.empty());
	buffers.caches[0].data.resize(geometry.vertices.size() * sizeof(decltype(geometry.vertices[0])));
	std::memcpy(buffers.caches[0].data.data(), geometry.vertices.data(), buffers.caches[0].data.size());
	if (!geometry.indices.empty()) {
		buffers.caches[1].data.resize(geometry.indices.size() * sizeof(decltype(geometry.indices[0])));
		std::memcpy(buffers.caches[1].data.data(), geometry.indices.data(), buffers.caches[1].data.size());
	}
}
} // namespace

GeometryBuffer::GeometryBuffer(Context const& context, std::string name) : GfxResource(context.vram(), std::move(name)) {
	auto& bufs = m_allocation->buffers.caches;
	bufs.push_back(context.vram());
	bufs.back().setVbo(context.vram().buffering);
	bufs.push_back(context.vram());
	bufs.back().setIbo(context.vram().buffering);
}

Result<void> GeometryBuffer::write(Geometry geometry) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }
	if (geometry.vertices.empty()) { return Error::eInvalidArgument; }

	writeGeometry(m_allocation->buffers, geometry);

	m_geometry = std::move(geometry);
	return Result<void>::success();
}

UniformBuffer::UniformBuffer(Context const& context, std::string name) : GfxResource(context.vram(), std::move(name)) {
	auto& bufs = m_allocation->buffers.caches;
	bufs.push_back(context.vram());
	bufs.back().info.usage = vk::BufferUsageFlagBits::eUniformBuffer;
	bufs.back().info.size = 1;
	bufs.back().buffers.push(context.vram().makeBuffer(bufs.back().info, true, m_allocation->name.c_str()));
}

std::size_t UniformBuffer::size() const { return m_allocation->buffers.caches.empty() ? 0 : m_allocation->buffers.caches[0].buffers.get()->size; }

Result<void> UniformBuffer::reserve(std::size_t const size) {
	if (!m_allocation || !m_allocation->vram || m_allocation->buffers.caches.empty()) { return Error::eInactiveInstance; }
	if (size == 0) { return Error::eInvalidArgument; }

	auto& buffer = m_allocation->buffers.caches[0].buffers.get();
	if (buffer->size < size) {
		m_allocation->vram.device.defer(std::move(buffer));
		buffer = m_allocation->vram.makeBuffer(m_allocation->buffers.caches[0].info, true, m_allocation->name.c_str());
	}

	if (!buffer) { return Error::eMemoryError; }
	return Result<void>::success();
}

Result<void> UniformBuffer::write(BufferWrite const data) {
	if (!m_allocation || !m_allocation->vram || m_allocation->buffers.caches.empty()) { return Error::eInactiveInstance; }
	if (!data.data || data.size == 0) { return Error::eInvalidArgument; }

	if (!reserve(data.size)) { return Error::eMemoryError; }
	if (!m_allocation->buffers.caches[0].buffers.get()->write(data.data, data.size)) { return Error::eMemoryError; }

	return Result<void>::success();
}
} // namespace vf
