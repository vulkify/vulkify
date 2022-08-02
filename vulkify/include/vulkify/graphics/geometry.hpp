#pragma once
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/core/rgba.hpp>
#include <vector>

namespace vf {
///
/// \brief A single vertex
///
/// Note: Vertex objects are uploaded directly to vertex buffers
///
struct Vertex {
	///
	/// \brief Vertex position
	///
	glm::vec2 xy;
	///
	/// \brief Texture coordinates
	///
	glm::vec2 uv;
	///
	/// \brief Vertex colour
	///
	glm::vec4 rgba;

	static constexpr Vertex make(glm::vec2 xy = {}, glm::vec2 uv = {}, glm::vec4 rgba = glm::vec4{1.0f}) { return {xy, uv, rgba}; }
};

///
/// \brief Spec for quad shape
///
struct QuadCreateInfo {
	glm::vec2 size{100.0f, 100.0f};
	glm::vec2 origin{};
	UvRect uv{};
	Rgba vertex{white_v};
};

///
/// \brief Spec for regular polygon
///
struct PolygonCreateInfo {
	float diameter{100.0f};
	std::uint32_t points{64};
	glm::vec2 origin{};
	Rgba vertex{white_v};
};

///
/// \brief Graphics geometry (
///
/// Note: Geometry objects are uploaded directly to vertex/index buffers
///
struct Geometry {
	std::vector<Vertex> vertices{};
	std::vector<std::uint32_t> indices{};

	void add(std::span<Vertex const> v, std::span<std::uint32_t const> i);
	void add_quad(QuadCreateInfo const& info);

	void reserve(std::size_t verts, std::size_t inds);

	static Geometry make_quad(QuadCreateInfo const& info = {});
	static Geometry make_regular_polygon(PolygonCreateInfo const& info = {});
};
} // namespace vf
