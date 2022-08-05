#pragma once
#include <ktl/fixed_any.hpp>

namespace vf {
struct TextureHandle {
	ktl::fixed_any<2 * sizeof(void*)> handle{};

	bool operator==(TextureHandle const& rhs) const;
	explicit operator bool() const;
};
} // namespace vf
