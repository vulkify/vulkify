#pragma once
#include <ktl/kunique_ptr.hpp>

namespace vf::refactor {
struct GfxDevice;

class GfxAllocation {
  public:
	enum class Type { eBuffer, eImage };

	virtual ~GfxAllocation() = 0;
	GfxAllocation& operator=(GfxAllocation&&) = delete;

	GfxDevice const& device() const { return *m_device; }
	Type type() const { return m_type; }

  protected:
	GfxAllocation(GfxDevice const& device, Type type) : m_device(&device), m_type(type) {}

  private:
	GfxDevice const* m_device{};
	Type m_type{};
};

template <std::derived_from<GfxAllocation> T>
ktl::kunique_ptr<T> make_allocation(GfxDevice const& device) {
	return ktl::make_unique<T>(device);
}
} // namespace vf::refactor
