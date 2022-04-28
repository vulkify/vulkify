#pragma once
#include <glm/mat4x4.hpp>

namespace vf {
struct DrawModel {
	glm::vec4 pos_rot{};
	glm::vec4 scl{1.0f};
	glm::vec4 tint{1.0f};
};
} // namespace vf
