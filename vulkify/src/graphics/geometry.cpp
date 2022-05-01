#include <vulkify/graphics/geometry.hpp>

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
} // namespace vf
