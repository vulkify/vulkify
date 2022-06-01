#include <detail/shared_impl.hpp>
#include <vulkify/context/context.hpp>
#include <vulkify/graphics/buffer.hpp>

namespace vf {
namespace {
Geometry fromBytes(std::span<std::byte const> verts, std::span<std::byte const> idxs) {
	auto ret = Geometry{};
	if (verts.size() > 1) {
		assert(verts.size() % sizeof(decltype(ret.vertices[0])) == 0);
		ret.vertices.resize(verts.size() / sizeof(decltype(ret.vertices[0])));
		std::memcpy(ret.vertices.data(), verts.data(), verts.size());
	}
	if (idxs.size() > 1) {
		assert(idxs.size() % sizeof(decltype(ret.indices[0])) == 0);
		ret.indices.resize(idxs.size() / sizeof(decltype(ret.indices[0])));
		std::memcpy(ret.indices.data(), idxs.data(), idxs.size());
	}
	return ret;
}

void writeGeometry(BufferCache& vbo, BufferCache& ibo, Geometry const& geometry) {
	assert(!geometry.vertices.empty());
	vbo.data.resize(geometry.vertices.size() * sizeof(decltype(geometry.vertices[0])));
	std::memcpy(vbo.data.data(), geometry.vertices.data(), vbo.data.size());
	if (!geometry.indices.empty()) {
		ibo.data.resize(geometry.indices.size() * sizeof(decltype(geometry.indices[0])));
		std::memcpy(ibo.data.data(), geometry.indices.data(), ibo.data.size());
	}
}
} // namespace

GeometryBuffer::GeometryBuffer(Context const& context, std::string name) : GfxResource(context.vram(), std::move(name)) {
	auto& bufs = m_allocation->buffers;
	auto const& vram = context.vram();
	bufs[0] = BufferCache(vram, vk::BufferUsageFlagBits::eVertexBuffer);
	bufs[1] = BufferCache(vram, vk::BufferUsageFlagBits::eIndexBuffer);
}

Geometry GeometryBuffer::geometry() const {
	if (!m_allocation || !m_allocation->vram) { return {}; }
	return fromBytes(m_allocation->buffers[0].data, m_allocation->buffers[1].data);
}

Result<void> GeometryBuffer::write(Geometry geometry) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }
	if (geometry.vertices.empty()) { return Error::eInvalidArgument; }

	writeGeometry(m_allocation->buffers[0], m_allocation->buffers[1], geometry);
	m_counts.vertices = static_cast<std::uint32_t>(geometry.vertices.size());
	m_counts.indices = static_cast<std::uint32_t>(geometry.indices.size());

	return Result<void>::success();
}

UniformBuffer::UniformBuffer(Context const& context, std::string name) : GfxResource(context.vram(), std::move(name)) {
	auto& bufs = m_allocation->buffers;
	bufs[0] = BufferCache(context.vram(), vk::BufferUsageFlagBits::eUniformBuffer);
}

std::size_t UniformBuffer::size() const { return m_allocation->buffers[0].buffers.get()->size; }

Result<void> UniformBuffer::reserve(std::size_t const size) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }
	if (size == 0) { return Error::eInvalidArgument; }

	auto& cache = m_allocation->buffers[0];
	auto& buffer = cache.buffers.get();
	if (buffer->size < size) {
		m_allocation->vram.device.defer(std::move(cache));
		buffer = m_allocation->vram.makeBuffer(cache.info, true, m_allocation->name.c_str());
	}

	if (!buffer) { return Error::eMemoryError; }
	return Result<void>::success();
}

Result<void> UniformBuffer::write(void const* data, std::size_t const size) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }
	if (!reserve(size)) { return Error::eMemoryError; }
	if (!m_allocation->buffers[0].buffers.get()->write(data, size)) { return Error::eMemoryError; }

	return Result<void>::success();
}
} // namespace vf
