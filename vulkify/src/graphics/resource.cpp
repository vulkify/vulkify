#include <detail/shared_impl.hpp>
#include <vulkify/graphics/resource.hpp>

namespace vf {
static void deferBuffer(Vram const& vram, BufferCache&& buffer) {
	if (vram && buffer) { vram.device.defer(std::move(buffer)); }
}

GfxResource::GfxResource() : m_allocation(ktl::make_unique<GfxAllocation>()) {}
GfxResource::GfxResource(GfxResource&&) noexcept = default;
GfxResource& GfxResource::operator=(GfxResource&&) noexcept = default;
GfxResource::~GfxResource() {
	if (m_allocation) { deferBuffer(m_allocation->vram, std::move(m_allocation->buffer)); }
}

GfxResource::GfxResource(Vram const& vram, std::string name) : m_name(std::move(name)), m_allocation(ktl::make_unique<GfxAllocation>()) {
	m_allocation->vram = vram;
	m_allocation->image = {{vram, m_name}};
}

GfxResource::operator bool() const { return static_cast<bool>(vram()) && m_allocation; }

Vram const& GfxResource::vram() const { return m_allocation->vram; }

void GfxResource::defer() && { deferBuffer(m_allocation->vram, std::move(m_allocation->buffer)); }
} // namespace vf
