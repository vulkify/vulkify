#pragma once
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/primitives/mesh_primitive.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
///
/// \brief Primitive with public GeometryBuffer, Ptr<Texture const>, and DrawInstance
///
class Mesh : public MeshPrimitive {
  public:
	static Mesh makeQuad(Context const& context, std::string name, QuadCreateInfo const& info = {}, Ptr<Texture const> texture = {});

	Mesh() = default;
	Mesh(Context const& context, std::string name);

	Drawable drawable() const { return {{&instance, 1}, gbo, texture}; }
	void draw(Surface const& surface) const override;

	DrawInstance instance{};
};
} // namespace vf
