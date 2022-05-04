#pragma once
#include <vulkify/core/rect.hpp>
#include <vulkify/core/transform.hpp>

namespace vf {
struct View {
	Transform transform{};
	Rect viewport{{1.0f, 1.0f}, {0.0f, 0.0f}};
};
} // namespace vf
