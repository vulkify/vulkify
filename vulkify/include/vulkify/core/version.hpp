#pragma once
#include <compare>

namespace vf {
struct Version {
	int major{};
	int minor{};
	int patch{};

	auto operator<=>(Version const&) const = default;

	template <typename OstreamT>
	friend constexpr OstreamT& operator<<(OstreamT& out, Version const& v) {
		return out << 'v' << v.major << '.' << v.minor << '.' << v.patch;
	}

	template <typename IstreamT>
	friend constexpr IstreamT& operator<<(IstreamT& in, Version& out) {
		char discard;
		return in >> discard >> out.major >> discard >> out.minor >> discard >> out.patch;
	}
};
} // namespace vf
