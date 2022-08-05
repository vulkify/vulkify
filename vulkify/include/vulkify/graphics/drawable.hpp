#pragma once
#include <vulkify/core/ptr.hpp>
#include <vulkify/core/rgba.hpp>
#include <vulkify/core/transform.hpp>
#include <vulkify/graphics/draw_model.hpp>
#include <vulkify/graphics/resources/texture_handle.hpp>
#include <cstring>

namespace vf {
class GeometryBuffer;

struct DrawInstance {
	Transform transform{};
	Rgba tint = white_v;

	DrawModel draw_model() const;
	operator std::span<DrawInstance const>() const { return {this, 1}; }
};

///
/// \brief View to geometry, texture, and instances associated with a single draw call
///
struct Drawable {
	std::span<DrawInstance const> instances{};
	GeometryBuffer const& buffer;
	TextureHandle texture{};
};

// impl

inline DrawModel DrawInstance::draw_model() const {
	auto ret = DrawModel{};
	ret.pos_orn = {transform.position, transform.orientation.value()};
	auto const utint = tint.to_u32();
	float ftint;
	std::memcpy(&ftint, &utint, sizeof(float));
	ret.scl_tint = {transform.scale, ftint, 0.0f};
	return ret;
}
} // namespace vf
