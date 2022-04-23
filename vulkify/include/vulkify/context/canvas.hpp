#pragma once
#include <ktl/fixed_pimpl.hpp>

namespace vf {
class Canvas {
  public:
	struct Impl;

	Canvas() noexcept;
	Canvas(Impl impl);
	Canvas(Canvas&&) noexcept;
	Canvas& operator=(Canvas&&) noexcept;
	~Canvas() noexcept;

	explicit operator bool() const;

  private:
	ktl::fixed_pimpl<Impl, 64> m_impl;
	friend class Context;
};
} // namespace vf
