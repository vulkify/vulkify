#pragma once
#include <ktl/kunique_ptr.hpp>
#include <string>

namespace vf {
struct GfxAllocation;
struct Vram;

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
	GfxAllocation const& resource() const { return *m_allocation; }
	std::string const& name() const { return m_name; }

	void defer() &&;

  protected:
	Vram const& vram() const;

	std::string m_name{};
	ktl::kunique_ptr<GfxAllocation> m_allocation;
};
} // namespace vf
