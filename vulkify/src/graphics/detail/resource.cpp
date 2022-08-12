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
void defer_alloc(GfxDevice const* device, Handle<GfxAllocation> handle) {
	if (!device || !handle) { return; }
	auto lock = std::scoped_lock(device->allocations->mutex);
	auto* alloc = device->allocations->find(lock, {handle.value});
	if (!alloc) { return; }
	device->defer->push(std::move(*alloc));
	device->allocations->remove(lock, {handle.value});
}
} // namespace

GfxResource::~GfxResource() { defer_alloc(m_device, m_allocation); }

void GfxResource::release() && {
	defer_alloc(m_device, m_allocation);
	m_allocation = {};
}
} // namespace vf::refactor
