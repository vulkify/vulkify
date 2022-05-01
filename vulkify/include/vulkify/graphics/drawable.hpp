#pragma once
#include <vulkify/core/rgba.hpp>
#include <vulkify/core/transform.hpp>
#include <vulkify/graphics/draw_model.hpp>

namespace vf {
class GeometryBuffer;
class Texture;

struct DrawInstance {
	Transform transform{};
	Rgba tint = white_v;

	DrawModel drawModel() const;
};

struct Drawable {
	std::span<DrawInstance const> instances{};
	GeometryBuffer const& gbo;
	Texture const& texture;
};

// impl

inline DrawModel DrawInstance::drawModel() const {
	auto ret = DrawModel{};
	ret.pos_orn = {transform.position, static_cast<glm::vec2>(transform.orientation)};
	auto const utint = tint.toU32();
	ret.scl_tint = {transform.scale, *reinterpret_cast<float const*>(&utint), 0.0f};
	return ret;
}
} // namespace vf
