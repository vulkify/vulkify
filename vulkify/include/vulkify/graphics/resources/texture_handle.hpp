#pragma once
#include <ktl/fixed_any.hpp>

namespace vf {
struct TextureHandle {
	ktl::fixed_any<2 * sizeof(void*)> handle{};

	explicit operator bool() const { return !handle.empty(); }
};
} // namespace vf
