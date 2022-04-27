#pragma once
#include <glm/mat4x4.hpp>
#include <vulkify/core/rgba.hpp>

namespace vf {
struct DrawParams {
	glm::mat4 modelMatrix{};
	Rgba tint{white_v};
	char const* name{""};
};
} // namespace vf
