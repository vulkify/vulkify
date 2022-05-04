#pragma once
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace vf {
struct VideoMode {
	glm::uvec2 resolution{};
	glm::uvec3 bitDepth{};
	std::uint32_t refreshRate{};
};
} // namespace vf
