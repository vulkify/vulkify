#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace vf {
struct VideoMode {
	glm::uvec2 resolution{};
	glm::uvec3 bit_depth{};
	std::uint32_t refresh_rate{};
};
} // namespace vf
