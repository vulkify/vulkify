#pragma once
#include <vulkify/core/rgba.hpp>
#include <vulkify/core/transform.hpp>

namespace vf {
struct DrawModel {
	glm::vec4 pos_rot{};
	glm::vec4 scl{1.0f};
	glm::vec4 tint{1.0f};

	static DrawModel make(Transform const& transform, Rgba const rgba) {
		auto ret = DrawModel{};
		ret.pos_rot = {transform.position, static_cast<glm::vec2 const&>(transform.orientation)};
		ret.scl = {transform.scale, 0.0f, 0.0f};
		ret.tint = rgba.normalize();
		return ret;
	}
};
} // namespace vf
