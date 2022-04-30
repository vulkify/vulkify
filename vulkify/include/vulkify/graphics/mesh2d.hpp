#pragma once
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
struct Mesh2D {
	GeometryBuffer vbo{};
	Texture texture{};
	Primitive::Instance primitive{};

	static Mesh2D make(Vram const& vram, std::string name) { return {{vram, std::move(name)}}; }

	Primitive getPrimitive() const { return {{&primitive, 1}, &vbo, &texture}; }
};

template <typename Storage = std::vector<Primitive::Instance>>
struct InstancedMesh2D {
	GeometryBuffer vbo{};
	Texture texture{};
	Storage instances{};

	static InstancedMesh2D make(Vram const& vram, std::string name) { return {{vram, std::move(name)}}; }

	Primitive getPrimitive() const { return {{instances.data(), instances.size()}, &vbo, &texture}; }
};
} // namespace vf
