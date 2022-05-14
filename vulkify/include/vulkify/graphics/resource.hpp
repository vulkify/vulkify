#pragma once
#include <ktl/kunique_ptr.hpp>
#include <string>

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

	GfxResource(Vram const& vram, std::string name);

	explicit operator bool() const;
	GfxAllocation const& resource() const;
	std::string const& name() const;

	void release() &&;

  protected:
	Vram const& vram() const;

	ktl::kunique_ptr<GfxAllocation> m_allocation;
};
} // namespace vf
