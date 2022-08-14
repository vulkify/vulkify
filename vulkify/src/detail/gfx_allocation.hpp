#pragma once
#include <detail/defer_base.hpp>
#include <vulkify/core/ptr.hpp>

namespace vf {
struct GfxDevice;

class GfxAllocation : public DeferBase {
  public:
	enum class Type { eBuffer, eImage, eFont, eShader };

	virtual ~GfxAllocation() = default;
	GfxAllocation& operator=(GfxAllocation&&) = delete;

	Ptr<GfxDevice const> device() const { return m_device; }
	Type type() const { return m_type; }

	explicit operator bool() const { return m_device != nullptr; }

  protected:
	GfxAllocation(GfxDevice const* device, Type type) : m_device(device), m_type(type) {}

  private:
	GfxDevice const* m_device{};
	Type m_type{};
};
} // namespace vf
