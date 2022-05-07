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
/// \brief Data structure specifying a quad shape
///
struct QuadCreateInfo {
	glm::vec2 size{100.0f, 100.0f};
	glm::vec2 origin{};
	QuadUV uv{};
	Rgba vertex{white_v};
};

///
/// \brief Data structure specifying a regular polygon
///
struct PolygonCreateInfo {
	float diameter{100.0f};
	std::uint32_t points{64};
	glm::vec2 origin{};
	Rgba vertex{white_v};
};

///
/// \brief Data structure specifying graphics geometry (
///
/// Note: Geometry objects are uploaded directly to vertex/index buffers
///
struct Geometry {
	std::vector<Vertex> vertices{};
	std::vector<std::uint32_t> indices{};

	static Geometry makeQuad(QuadCreateInfo const& info = {});
	static Geometry makeRegularPolygon(PolygonCreateInfo const& info = {});
};
} // namespace vf
