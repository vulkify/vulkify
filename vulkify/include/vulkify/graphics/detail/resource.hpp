#pragma once
#include <ktl/kunique_ptr.hpp>

namespace vf {
struct GfxAllocation;
struct Vram;

///
/// \brief Low level GPU resource allocation
///
class GfxResource {
  public:
	GfxResource();
	GfxResource(GfxResource&&) noexcept;
	GfxResource& operator=(GfxResource&&) noexcept;
	virtual ~GfxResource();

	GfxResource(GfxResource const&) = delete;
	GfxResource& operator=(GfxResource const&) = delete;

	explicit GfxResource(Vram const& vram);

	explicit operator bool() const;
	GfxAllocation const& resource() const;

  protected:
	Vram const& vram() const;
	void release() &&;

	ktl::kunique_ptr<GfxAllocation> m_allocation;
};
} // namespace vf

#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/ptr.hpp>

namespace vf::refactor {
struct GfxDevice;
struct GfxAllocation;

///
/// \brief Low level GPU resource allocation
///
class GfxResource {
  public:
	GfxResource();
	GfxResource(GfxResource&&) noexcept;
	GfxResource& operator=(GfxResource&&) noexcept;
	virtual ~GfxResource();

	GfxResource(GfxResource const&) = delete;
	GfxResource& operator=(GfxResource const&) = delete;

	explicit operator bool() const;
	Ptr<GfxAllocation const> allocation() const;

  protected:
	GfxResource(ktl::kunique_ptr<GfxAllocation>&& allocation);

	GfxDevice const& device() const;
	void release() &&;

	ktl::kunique_ptr<GfxAllocation> m_allocation;
};
} // namespace vf::refactor
