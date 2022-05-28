#pragma once
#include <concepts>

namespace vf {
///
/// \brief Epsilon based floating point equality comparator
///
template <std::floating_point Type = float>
struct FloatEq {
	using type = Type;

	Type epsilson = static_cast<Type>(0.0001f);

	static constexpr Type abs(float x) { return x < Type{} ? -x : x; }

	constexpr bool operator()(Type const a, Type const b) const {
		auto const fa = abs(a);
		auto const fb = abs(b);
		if (fa == fb) { return true; }
		auto const diff = abs(a - b);
		if (fa < 1.0f || fb < 1.0f) { return diff < epsilson; }
		return diff / (fb + fb) < epsilson;
	}
};
} // namespace vf
