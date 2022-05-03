#pragma once

namespace vf {
struct Version {
	int major{};
	int minor{};
	int patch{};

	auto operator<=>(Version const&) const = default;
};
} // namespace vf
