#pragma once
#include <vulkify/context/context.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/primitives/mesh_primitive.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
template <typename T>
concept InstancedMeshStorage = std::convertible_to<T, std::span<DrawInstance const>>;

///
/// \brief Primitive with public GeometryBuffer, Texture, and std::vector<DrawInstance> (customizable)
///
template <InstancedMeshStorage Storage = std::vector<DrawInstance>>
class InstancedMesh : public MeshPrimitive {
  public:
	InstancedMesh() = default;
	InstancedMesh(Context const& context, std::string name) : MeshPrimitive(context, std::move(name)) {}

	Drawable drawable() const { return {instances, gbo, texture}; }

	void draw(Surface const& surface) const override { surface.draw(drawable()); }

	Texture texture{};
	Storage instances{};
};
} // namespace vf
