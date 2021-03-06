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

	DrawModel drawModel() const;
};

///
/// \brief View to geometry, texture, and instances associated with a single draw call
///
struct Drawable {
	std::span<DrawInstance const> instances{};
	GeometryBuffer const& gbo;
	TextureHandle texture{};
};

// impl

inline DrawModel DrawInstance::drawModel() const {
	auto ret = DrawModel{};
	ret.pos_orn = {transform.position, transform.orientation.value()};
	auto const utint = tint.toU32();
	float ftint;
	std::memcpy(&ftint, &utint, sizeof(float));
	ret.scl_tint = {transform.scale, ftint, 0.0f};
	return ret;
}
} // namespace vf
