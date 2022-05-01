#pragma once
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
class Mesh2D : public Primitive {
  public:
	Mesh2D() = default;
	Mesh2D(Vram const& vram, std::string name) : gbo(vram, std::move(name)) {}

	Drawable drawable() const override { return {{&instance, 1}, gbo, texture}; }

	GeometryBuffer gbo{};
	Texture texture{};
	DrawInstance instance{};
};

template <typename Storage = std::vector<DrawInstance>>
class InstancedMesh2D : public Primitive {
  public:
	InstancedMesh2D() = default;
	InstancedMesh2D(Vram const& vram, std::string name) : gbo(vram, std::move(name)) {}

	Drawable drawable() const override { return {instances, gbo, texture}; }

	GeometryBuffer gbo{};
	Texture texture{};
	Storage instances{};
};
} // namespace vf
