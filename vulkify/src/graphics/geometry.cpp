#include <vulkify/core/radian.hpp>
#include <vulkify/graphics/geometry.hpp>
#include <cmath>
#include <iterator>

namespace vf {
using v2 = glm::vec2;

void Geometry::add(std::span<Vertex const> v, std::span<std::uint32_t const> i) {
	reserve(v.size(), i.size());
	auto const offset = static_cast<std::uint32_t>(vertices.size());
	std::copy(v.begin(), v.end(), std::back_inserter(vertices));
	for (auto const index : i) { indices.push_back(index + offset); }
}

void Geometry::add_quad(QuadCreateInfo const& info) {
	auto const colour = info.vertex.normalize();
	auto const hs = info.size * 0.5f;
	Vertex const verts[] = {
		{info.origin + v2(-hs.x, hs.y), info.uv.top_left, colour},
		{info.origin + v2(-hs.x, -hs.y), {info.uv.top_left.x, info.uv.bottom_right.y}, colour},
		{info.origin + v2(hs.x, -hs.y), info.uv.bottom_right, colour},
		{info.origin + v2(hs.x, hs.y), {info.uv.bottom_right.x, info.uv.top_left.y}, colour},
	};
	std::uint32_t const idxs[] = {0, 1, 2, 2, 3, 0};
	add(verts, idxs);
}

void Geometry::reserve(std::size_t verts, std::size_t inds) {
	vertices.reserve(vertices.size() + verts);
	indices.reserve(indices.size() + inds);
}

Geometry Geometry::make_quad(QuadCreateInfo const& info) {
	auto ret = Geometry{};
	ret.add_quad(info);
	return ret;
}

Geometry Geometry::make_regular_polygon(PolygonCreateInfo const& info) {
	if (info.points < 3) { return {}; }
	auto ret = Geometry{};
	auto const colour = info.vertex.normalize();
	static constexpr auto uvx = glm::vec2(0.5f, -0.5f);
	auto const centre = Vertex{info.origin, {0.5f, 0.5f}, colour};
	auto const radius = info.diameter * 0.5f;
	auto makeVertex = [&](Radian rad) {
		auto const c = glm::vec2(std::cos(rad), std::sin(rad));
		return Vertex{centre.xy + c * radius, c * uvx + centre.uv, colour};
	};
	ret.reserve(info.points + 1, (info.points + 1) * 3);
	ret.vertices.push_back(centre);
	ret.vertices.push_back(makeVertex({0.0f}));
	for (std::uint32_t point = 1; point < info.points; ++point) {
		auto const rad = Degree{(static_cast<float>(point) / static_cast<float>(info.points)) * 360.0f};
		auto const next = static_cast<std::uint32_t>(ret.vertices.size());
		ret.vertices.push_back(makeVertex(rad));
		std::uint32_t const indices[] = {0, next - 1, next};
		std::copy(std::begin(indices), std::end(indices), std::back_inserter(ret.indices));
	}
	std::uint32_t const indices[] = {0, static_cast<std::uint32_t>(ret.vertices.size() - 1), 1};
	std::copy(std::begin(indices), std::end(indices), std::back_inserter(ret.indices));
	return ret;
}
} // namespace vf
