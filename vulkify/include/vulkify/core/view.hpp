#pragma once
#include <vulkify/core/nvec.hpp>

namespace vf {
struct View {
	glm::vec2 position{};
	nvec2 orientation{};
};
} // namespace vf
