#pragma once
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <vulkify/core/rgba.hpp>
#include <vector>

namespace vf {
struct Vertex {
	glm::vec2 xy{};
	glm::vec2 uv{};
	glm::vec4 rgba{};
};

struct Geometry {
	std::vector<Vertex> vertices{};
	std::vector<std::uint32_t> indices{};
};

struct QuadUV {
	glm::vec2 topLeft{0.0f, 0.0f};
	glm::vec2 bottomRight{1.0f, 1.0f};
};

Geometry makeQuad(glm::vec2 size, glm::vec2 origin = {}, Rgba rgba = white_v, QuadUV const& uv = {});
} // namespace vf
