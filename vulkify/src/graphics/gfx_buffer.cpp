#include <ktl/fixed_pimpl.hpp>

namespace vf {
struct GfxResource;
struct Vram;

class GfxBuffer {
  public:
	GfxBuffer() noexcept;
	GfxBuffer(GfxBuffer&&) noexcept;
	GfxBuffer& operator=(GfxBuffer&&) noexcept;
	virtual ~GfxBuffer() noexcept;

	GfxBuffer(GfxBuffer const&) = delete;
	GfxBuffer& operator=(GfxBuffer const&) = delete;

	GfxBuffer(Vram vram);

	explicit operator bool() const;

  protected:
	Vram const& vram() const;

	ktl::fixed_pimpl<GfxResource, 128> m_resource;
};
} // namespace vf

#include <vulkify/core/geometry.hpp>

namespace vf {
class VertexBuffer : public GfxBuffer {
  public:
	VertexBuffer() = default;

	VertexBuffer(Vram vram, std::size_t size);
};
} // namespace vf

#include <detail/vram.hpp>

namespace vf {
GfxBuffer::GfxBuffer() noexcept = default;
GfxBuffer::GfxBuffer(GfxBuffer&&) noexcept = default;
GfxBuffer& GfxBuffer::operator=(GfxBuffer&&) noexcept = default;
GfxBuffer::~GfxBuffer() noexcept = default;

GfxBuffer::GfxBuffer(Vram vram) { m_resource->vram = vram; }

GfxBuffer::operator bool() const { return static_cast<bool>(vram()); }

Vram const& GfxBuffer::vram() const { return m_resource->vram; }

VertexBuffer::VertexBuffer(Vram vram, std::size_t size) : GfxBuffer(vram) {
	if (vram) {
		auto info = vk::BufferCreateInfo({}, size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer);
		m_resource->resource = vram.makeBuffer(info, VMA_MEMORY_USAGE_GPU_ONLY);
	}
}
} // namespace vf
