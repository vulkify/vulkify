#pragma once
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <vulkify/core/rgba.hpp>
#include <vector>

namespace vf {
///
/// \brief Data structure specifying a single vertex
///
/// Note: Vertex objects are uploaded directly to vertex buffers
///
struct Vertex {
	///
	/// \brief Vertex position
	///
	glm::vec2 xy{};
	///
	/// \brief Texture coordinates
	///
	glm::vec2 uv{};
	///
	/// \brief Vertex colour
	///
	glm::vec4 rgba{1.0f};
};

///
/// \brief Data structure specifying a quad's texture coordinates
///
struct QuadUV {
	glm::vec2 topLeft{0.0f, 0.0f};
	glm::vec2 bottomRight{1.0f, 1.0f};
};

///
/// \brief Data structure specifying graphics geometry (
///
/// Note: Geometry objects are uploaded directly to vertex/index buffers
///
struct Geometry {
	std::vector<Vertex> vertices{};
	std::vector<std::uint32_t> indices{};

	static Geometry makeQuad(glm::vec2 size, glm::vec2 origin = {}, Rgba rgba = white_v, QuadUV const& uv = {});
	static Geometry makeRegularPolygon(float diameter, std::uint32_t sides, glm::vec2 origin = {}, Rgba rgba = white_v);
};
} // namespace vf
