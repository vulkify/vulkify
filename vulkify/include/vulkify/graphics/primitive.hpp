#pragma once
#include <vulkify/core/rgba.hpp>
#include <vulkify/core/transform.hpp>
#include <vulkify/graphics/draw_model.hpp>

namespace vf {
class GeometryBuffer;
class Texture;
class Surface;

struct Primitive {
	static constexpr std::size_t small_buffer_v = 8;

	struct Instance {
		Transform transform{};
		Rgba tint = white_v;

		DrawModel drawModel() const;
	};

	std::span<Instance const> instances{};
	GeometryBuffer const* vbo{};
	Texture const* texture{};

	explicit operator bool() const { return !instances.empty() && vbo && texture; }

	void draw(Surface const& surface) const;
	Primitive getPrimitive() const { return *this; }

	template <std::output_iterator<DrawModel> Out>
	static void addDrawModels(std::span<Instance const> instances, Out out) {
		for (auto const& instance : instances) { *out++ = instance.drawModel(); }
	}
};

// impl

inline DrawModel Primitive::Instance::drawModel() const {
	auto ret = DrawModel{};
	ret.pos_orn = {transform.position, static_cast<glm::vec2>(transform.orientation)};
	auto const utint = tint.toU32();
	ret.scl_tint = {transform.scale, *reinterpret_cast<float const*>(&utint), 0.0f};
	return ret;
}
} // namespace vf
