#include <vulkify/core/radian.hpp>
#include <vulkify/graphics/geometry.hpp>
#include <cmath>
#include <iterator>

namespace vf {
using v2 = glm::vec2;

Geometry Geometry::makeQuad(glm::vec2 const size, glm::vec2 const origin, Rgba const rgba, QuadUV const& uv) {
	auto ret = Geometry{};
	auto const colour = rgba.normalize();
	auto const hs = size * 0.5f;
	ret.vertices = {
		{origin + v2(-hs.x, hs.y), uv.topLeft, colour},
		{origin + v2(-hs.x, -hs.y), {uv.topLeft.x, uv.bottomRight.y}, colour},
		{origin + v2(hs.x, -hs.y), uv.bottomRight, colour},
		{origin + v2(hs.x, hs.y), {uv.bottomRight.x, uv.topLeft.y}, colour},
	};
	ret.indices = {0, 1, 2, 2, 3, 0};
	return ret;
}

Geometry Geometry::makeRegularPolygon(float diameter, std::uint32_t sides, glm::vec2 const origin, Rgba const rgba) {
	if (sides < 3) { return {}; }
	auto ret = Geometry{};
	auto const colour = rgba.normalize();
	static constexpr auto uvx = glm::vec2(0.5f, -0.5f);
	auto const centre = Vertex{origin, {0.5f, 0.5f}, colour};
	auto const radius = diameter * 0.5f;
	auto makeVertex = [&](Radian rad) {
		auto const c = glm::vec2(std::cos(rad), std::sin(rad));
		return Vertex{centre.xy + c * radius, c * uvx + centre.uv, colour};
	};
	ret.vertices.reserve(sides + 1);
	ret.indices.reserve((sides + 1) * 3);
	ret.vertices.push_back(centre);
	ret.vertices.push_back(makeVertex({0.0f}));
	for (std::uint32_t point = 1; point < sides; ++point) {
		auto const rad = Degree{(static_cast<float>(point) / static_cast<float>(sides)) * 360.0f};
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
