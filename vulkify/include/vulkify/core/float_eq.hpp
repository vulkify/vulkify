#pragma once
#include <concepts>

namespace vf {
///
/// \brief Epsilon based floating point equality comparator
///
template <std::floating_point Type = float>
struct FloatEq {
	using type = Type;

	static constexpr Type epsilson_v = static_cast<Type>(0.0001f);

	static constexpr Type abs(float x) { return x < Type{} ? -x : x; }

	constexpr bool operator()(Type const a, Type const b) const { return abs(a - b) < epsilson_v; }
};
} // namespace vf
