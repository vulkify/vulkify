#pragma once
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/primitives/mesh_primitive.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
///
/// \brief Primitive with public GeometryBuffer, Texture, and DrawInstance
///
class Mesh : public MeshPrimitive {
  public:
	Mesh() = default;
	Mesh(Context const& context, std::string name);

	Drawable drawable() const { return {{&instance, 1}, gbo, texture}; }
	void draw(Surface const& surface) const override;

	Texture texture{};
	DrawInstance instance{};
};
} // namespace vf
