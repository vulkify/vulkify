#pragma once
#include <glm/mat4x4.hpp>

namespace vf {
struct DrawInstanceData {
	glm::mat4 model{1.0f};
	glm::vec4 tint{1.0f};
};
} // namespace vf
