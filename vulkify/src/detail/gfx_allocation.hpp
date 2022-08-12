#pragma once
#include <vulkify/core/ptr.hpp>

namespace vf::refactor {
struct GfxDevice;

class GfxAllocation {
  public:
	enum class Type { eBuffer, eImage, eFont };

	virtual ~GfxAllocation() = default;
	GfxAllocation& operator=(GfxAllocation&&) = delete;

	Ptr<GfxDevice const> device() const { return m_device; }
	Type type() const { return m_type; }

  protected:
	GfxAllocation(GfxDevice const* device, Type type) : m_device(device), m_type(type) {}

  private:
	GfxDevice const* m_device{};
	Type m_type{};
};
} // namespace vf::refactor
