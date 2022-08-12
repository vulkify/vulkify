#include <detail/gfx_allocation.hpp>
#include <detail/gfx_device.hpp>
#include <detail/shared_impl.hpp>
#include <vulkify/graphics/detail/resource.hpp>

namespace vf {
namespace {
void defer_alloc(ktl::kunique_ptr<GfxAllocation>&& alloc) {
	if (alloc && alloc->vram.device) {
		auto device = alloc->vram.device;
		device.defer(std::move(alloc));
	}
}
} // namespace

GfxResource::GfxResource() : m_allocation(ktl::make_unique<GfxAllocation>()) {}
GfxResource::GfxResource(GfxResource&&) noexcept = default;
GfxResource& GfxResource::operator=(GfxResource&&) noexcept = default;
GfxResource::~GfxResource() { defer_alloc(std::move(m_allocation)); }

GfxResource::GfxResource(Vram const& vram) : m_allocation(ktl::make_unique<GfxAllocation>(vram)) {}

GfxResource::operator bool() const { return m_allocation && m_allocation->vram; }

GfxAllocation const& GfxResource::resource() const { return m_allocation ? *m_allocation : g_inactive.alloc; }

Vram const& GfxResource::vram() const { return m_allocation ? m_allocation->vram : g_inactive.vram; }

void GfxResource::release() && { defer_alloc(std::move(m_allocation)); }
} // namespace vf

namespace vf::refactor {
namespace {
void defer_alloc(ktl::kunique_ptr<GfxAllocation>&& alloc) { alloc->device().device.defer(std::move(alloc)); }
} // namespace

GfxResource::GfxResource() = default;
GfxResource::GfxResource(GfxResource&&) noexcept = default;
GfxResource& GfxResource::operator=(GfxResource&&) noexcept = default;
GfxResource::~GfxResource() { defer_alloc(std::move(m_allocation)); }

GfxResource::GfxResource(ktl::kunique_ptr<GfxAllocation>&& allocation) : m_allocation(std::move(allocation)) {}

GfxResource::operator bool() const { return m_allocation && m_allocation->device(); }

Ptr<GfxAllocation const> GfxResource::allocation() const { return m_allocation.get(); }

GfxDevice const& GfxResource::device() const {
	static auto const s_inactive = GfxDevice{};
	return m_allocation ? m_allocation->device() : s_inactive;
}

void GfxResource::release() && { defer_alloc(std::move(m_allocation)); }
} // namespace vf::refactor
