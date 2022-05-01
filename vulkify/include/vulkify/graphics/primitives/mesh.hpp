#pragma once
#include <vulkify/graphics/primitives/mesh_primitive.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
class Context;

///
/// \brief Primitive with public GeometryBuffer, Texture, and DrawInstance
///
class Mesh : public MeshPrimitive {
  public:
	Mesh() = default;
	Mesh(Context const& context, std::string name);

	Drawable drawable() const override { return {{&instance, 1}, gbo, texture}; }

	Texture texture{};
	DrawInstance instance{};
};

} // namespace vf
