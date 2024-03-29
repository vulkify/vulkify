#pragma once
#include <vulkify/core/ptr.hpp>
#include <vulkify/core/rgba.hpp>
#include <vulkify/core/transform.hpp>
#include <vulkify/graphics/detail/draw_model.hpp>
#include <vulkify/graphics/handle.hpp>
#include <cstring>

namespace vf {
class GeometryBuffer;
class Texture;

struct DrawInstance {
	Transform transform{};
	float z_index{};
	Rgba tint = white_v;

	DrawModel draw_model() const;
	operator std::span<DrawInstance const>() const { return {this, 1}; }
};

///
/// \brief View to geometry, texture, and instances associated with a single draw call
///
struct Drawable {
	std::span<DrawInstance const> instances{};
	Handle<GeometryBuffer> buffer{};
	Handle<Texture> texture{};
};

// impl

inline DrawModel DrawInstance::draw_model() const {
	auto ret = DrawModel{};
	ret.pos_orn = {transform.position, transform.orientation.value()};
	auto const utint = tint.to_u32();
	float ftint;
	std::memcpy(&ftint, &utint, sizeof(float));
	ret.scl_z_tint = {transform.scale, z_index, ftint};
	return ret;
}
} // namespace vf
