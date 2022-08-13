#include <detail/shared_impl.hpp>
#include <vulkify/context/context.hpp>
#include <vulkify/graphics/geometry_buffer.hpp>

namespace vf {
namespace {
Geometry from_bytes(std::span<std::byte const> verts, std::span<std::byte const> idxs) {
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

void write_geometry(BufferCache& vbo, BufferCache& ibo, Geometry const& geometry) {
	assert(!geometry.vertices.empty());
	vbo.data.resize(geometry.vertices.size() * sizeof(decltype(geometry.vertices[0])));
	std::memcpy(vbo.data.data(), geometry.vertices.data(), vbo.data.size());
	if (!geometry.indices.empty()) {
		ibo.data.resize(geometry.indices.size() * sizeof(decltype(geometry.indices[0])));
		std::memcpy(ibo.data.data(), geometry.indices.data(), ibo.data.size());
	}
}
} // namespace

GeometryBuffer::GeometryBuffer(Context const& context) : GfxResource(context.vram()) {
	auto& bufs = m_allocation->buffers;
	auto const& vram = context.vram();
	bufs[0] = BufferCache(vram, vk::BufferUsageFlagBits::eVertexBuffer);
	bufs[1] = BufferCache(vram, vk::BufferUsageFlagBits::eIndexBuffer);
}

Geometry GeometryBuffer::geometry() const {
	if (!m_allocation || !m_allocation->vram) { return {}; }
	return from_bytes(m_allocation->buffers[0].data, m_allocation->buffers[1].data);
}

Result<void> GeometryBuffer::write(Geometry geometry) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }
	if (geometry.vertices.empty()) { return Error::eInvalidArgument; }

	write_geometry(m_allocation->buffers[0], m_allocation->buffers[1], geometry);
	m_counts.vertices = static_cast<std::uint32_t>(geometry.vertices.size());
	m_counts.indices = static_cast<std::uint32_t>(geometry.indices.size());

	return Result<void>::success();
}
} // namespace vf

#include <detail/gfx_buffer_image.hpp>

namespace vf::refactor {
namespace {
void write_geometry(BufferCache& vbo, BufferCache& ibo, Geometry const& geometry) {
	assert(!geometry.vertices.empty());
	vbo.data.resize(geometry.vertices.size() * sizeof(decltype(geometry.vertices[0])));
	std::memcpy(vbo.data.data(), geometry.vertices.data(), vbo.data.size());
	if (!geometry.indices.empty()) {
		ibo.data.resize(geometry.indices.size() * sizeof(decltype(geometry.indices[0])));
		std::memcpy(ibo.data.data(), geometry.indices.data(), ibo.data.size());
	}
}
} // namespace

GeometryBuffer::GeometryBuffer(Context const& context) : GfxDeferred(&context.device()) {
	auto allocation = ktl::make_unique<GfxGeometryBuffer>(m_device);
	auto& bufs = allocation->buffers;
	bufs[0] = BufferCache(m_device, vk::BufferUsageFlagBits::eVertexBuffer);
	bufs[1] = BufferCache(m_device, vk::BufferUsageFlagBits::eIndexBuffer);
}

Result<void> GeometryBuffer::write(Geometry geometry) {
	if (geometry.vertices.empty()) { return Error::eInvalidArgument; }
	auto* self = static_cast<GfxGeometryBuffer*>(m_allocation.get());
	if (!self) { return Error::eInactiveInstance; }
	assert(self->type() == GfxAllocation::Type::eBuffer && self->buffers[0] && self->buffers[1]);

	write_geometry(self->buffers[0], self->buffers[1], geometry);
	self->vertices = static_cast<std::uint32_t>(geometry.vertices.size());
	self->indices = static_cast<std::uint32_t>(geometry.indices.size());

	return Result<void>::success();
}

Geometry GeometryBuffer::geometry() const {
	auto const* self = static_cast<GfxGeometryBuffer const*>(m_allocation.get());
	if (!self) { return {}; }
	assert(self->type() == GfxAllocation::Type::eBuffer && self->buffers[0] && self->buffers[1]);
	return from_bytes(self->buffers[0].data, self->buffers[1].data);
}

Handle<GeometryBuffer> GeometryBuffer::handle() const { return {m_allocation.get()}; }
} // namespace vf::refactor
