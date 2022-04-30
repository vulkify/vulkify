#include <detail/shared_impl.hpp>
#include <vulkify/graphics/resource.hpp>

namespace vf {
namespace {
void deferAlloc(ktl::kunique_ptr<GfxAllocation>&& alloc) {
	if (alloc && alloc->vram.device) {
		auto device = alloc->vram.device;
		device.defer(std::move(alloc));
	}
}
} // namespace

GfxResource::GfxResource() : m_allocation(ktl::make_unique<GfxAllocation>()) {}
GfxResource::GfxResource(GfxResource&&) noexcept = default;
GfxResource& GfxResource::operator=(GfxResource&&) noexcept = default;
GfxResource::~GfxResource() { deferAlloc(std::move(m_allocation)); }

GfxResource::GfxResource(Vram const& vram, std::string name) : m_allocation(ktl::make_unique<GfxAllocation>(vram, std::move(name))) {}

GfxResource::operator bool() const { return m_allocation && m_allocation->vram; }

GfxAllocation const& GfxResource::resource() const { return m_allocation ? *m_allocation : g_inactive.alloc; }
std::string const& GfxResource::name() const { return m_allocation ? m_allocation->name : g_inactive.name; }

Vram const& GfxResource::vram() const { return m_allocation ? m_allocation->vram : g_inactive.vram; }

void GfxResource::release() && { deferAlloc(std::move(m_allocation)); }
} // namespace vf
