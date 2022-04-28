#pragma once

namespace vf {
struct FloatEq {
	static constexpr float epsilson_v = 0.0001f;

	static constexpr float abs(float x) { return x < 0.0f ? -x : x; }

	constexpr bool operator()(float const a, float const b) const { return abs(a - b) < epsilson_v; }
};
} // namespace vf
