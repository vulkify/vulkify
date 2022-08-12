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

namespace vf::refactor {
struct GfxDevice;
class GfxAllocation;

///
/// \brief Low level GPU resource allocation
///
class GfxResource {
  public:
	GfxResource() noexcept;
	GfxResource(GfxResource&&) noexcept;
	GfxResource& operator=(GfxResource&&) noexcept;
	virtual ~GfxResource();

	GfxResource(GfxResource const&) = delete;
	GfxResource& operator=(GfxResource const&) = delete;

	explicit operator bool() const { return m_allocation != nullptr; }

  protected:
	GfxResource(GfxDevice const* device) noexcept;

	void release() &&;

	ktl::kunique_ptr<GfxAllocation> m_allocation{};
	GfxDevice const* m_device{};
};
} // namespace vf::refactor
