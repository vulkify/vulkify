#pragma once
#include <ktl/unique_val.hpp>
#include <vulkify/core/ptr.hpp>

namespace vf {
struct GfxDevice;

///
/// \brief Interface indicating a GPU resource: requires a GfxDevice to be active
///
class GfxResource {
  public:
	GfxResource() = default;
	virtual ~GfxResource() = default;

	explicit operator bool() const { return m_device.value; }

  protected:
	GfxResource(GfxDevice const* device) : m_device(device) {}

	ktl::unique_val<Ptr<GfxDevice const>> m_device{};
};
} // namespace vf
