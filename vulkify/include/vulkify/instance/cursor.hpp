#pragma once
#include <cstdint>

namespace vf {
enum class CursorMode { eStandard, eHidden, eDisabled };

struct Cursor {
	std::uint64_t handle{};

	constexpr bool operator==(Cursor const&) const = default;
};
} // namespace vf
