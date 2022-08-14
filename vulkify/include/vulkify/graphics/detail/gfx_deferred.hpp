#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/graphics/gfx_resource.hpp>

namespace vf {
///
/// \brief GPU resource owning a GfxAllocation whose destruction will be deferred
///
class GfxDeferred : public GfxResource {
  public:
	GfxDeferred() noexcept;
	GfxDeferred(GfxDeferred&&) noexcept;
	GfxDeferred& operator=(GfxDeferred&&) noexcept;
	~GfxDeferred() override;

	GfxDeferred(GfxDeferred const&) = delete;
	GfxDeferred& operator=(GfxDeferred const&) = delete;

  protected:
	GfxDeferred(Ptr<GfxDevice const> device) noexcept;

	void release() &&;

	ktl::kunique_ptr<class GfxAllocation> m_allocation{};
};
} // namespace vf
