#pragma once
#include <vulkify/core/ptr.hpp>
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

///
/// \brief View to geometry, texture, and instances associated with a single draw call
///
struct Drawable {
	std::span<DrawInstance const> instances{};
	GeometryBuffer const& gbo;
	Ptr<Texture const> texture{};
};

// impl

inline DrawModel DrawInstance::drawModel() const {
	auto ret = DrawModel{};
	ret.pos_orn = {transform.position, transform.orientation.value()};
	auto const utint = tint.toU32();
	ret.scl_tint = {transform.scale, *reinterpret_cast<float const*>(&utint), 0.0f};
	return ret;
}
} // namespace vf
