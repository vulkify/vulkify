#pragma once
#include <vulkify/graphics/primitives/mesh_primitive.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
class Mesh2D : public MeshPrimitive {
  public:
	using MeshPrimitive::MeshPrimitive;

	Drawable drawable() const override { return {{&instance, 1}, gbo, texture}; }

	Texture texture{};
	DrawInstance instance{};
};

template <typename Storage = std::vector<DrawInstance>>
class InstancedMesh2D : public MeshPrimitive {
  public:
	using MeshPrimitive::MeshPrimitive;

	Drawable drawable() const override { return {instances, gbo, texture}; }

	Texture texture{};
	Storage instances{};
};
} // namespace vf
