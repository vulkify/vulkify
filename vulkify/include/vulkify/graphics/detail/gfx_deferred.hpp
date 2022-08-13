#pragma once
#include <ktl/kunique_ptr.hpp>

namespace vf {
struct GfxDevice;
class GfxAllocation;

///
/// \brief Low level GPU resource allocation
///
class GfxDeferred {
  public:
	GfxDeferred() noexcept;
	GfxDeferred(GfxDeferred&&) noexcept;
	GfxDeferred& operator=(GfxDeferred&&) noexcept;
	virtual ~GfxDeferred();

	GfxDeferred(GfxDeferred const&) = delete;
	GfxDeferred& operator=(GfxDeferred const&) = delete;

	explicit operator bool() const { return m_allocation != nullptr; }

  protected:
	GfxDeferred(GfxDevice const* device) noexcept;

	void release() &&;

	ktl::kunique_ptr<GfxAllocation> m_allocation{};
	GfxDevice const* m_device{};
};
} // namespace vf
