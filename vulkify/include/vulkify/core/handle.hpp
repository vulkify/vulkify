#pragma once
#include <cstdint>

namespace vf {
template <typename Type>
struct Handle {
	using type = Type;
	using value_type = std::uint64_t;

	value_type value{};

	constexpr bool operator==(Handle const& rhs) const = default;
	explicit constexpr operator bool() const { return value > value_type{}; }
};
} // namespace vf
