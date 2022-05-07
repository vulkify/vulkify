#pragma once
#include <vulkify/core/nvec.hpp>
#include <vulkify/core/rect.hpp>

namespace vf {
struct View {
	glm::vec2 position{};
	nvec2 orientation{};
	Rect viewport{{1.0f, 1.0f}, {0.0f, 0.0f}};
};
} // namespace vf
