#include <vulkify/core/geometry.hpp>

using v2 = glm::vec2;

auto vf::makeQuad(glm::vec2 const size, glm::vec2 const origin, Rgba const rgba, QuadUV const& uv) -> Geometry {
	auto ret = Geometry{};
	auto const colour = rgba.normalize();
	auto const hs = size * 0.5f;
	ret.vertices = {
		{colour, origin + v2(-hs.x, hs.y), uv.topLeft},
		{colour, origin + v2(-hs.x, -hs.y), {uv.topLeft.x, uv.bottomRight.y}},
		{colour, origin + v2(hs.x, -hs.y), uv.bottomRight},
		{colour, origin + v2(hs.x, hs.y), {uv.bottomRight.x, uv.topLeft.y}},
	};
	ret.indices = {0, 1, 2, 2, 3, 0};
	return ret;
}