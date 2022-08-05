#pragma once
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/primitives/mesh_primitive.hpp>

namespace vf {
///
/// \brief Primitive with public GeometryBuffer, TextureHandle, and DrawInstance
///
class Mesh : public MeshPrimitive {
  public:
	static Mesh make_quad(Context const& context, std::string name, QuadCreateInfo const& info = {}, TextureHandle texture = {});

	Mesh() = default;
	Mesh(Context const& context, std::string name, TextureHandle texture = {});

	Drawable drawable() const { return {{&instance, 1}, gbo, texture}; }
	void draw(Surface const& surface, RenderState const& state = {}) const override;

	DrawInstance instance{};
};
} // namespace vf
