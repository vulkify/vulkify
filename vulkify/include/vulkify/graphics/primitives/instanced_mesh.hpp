#pragma once
#include <vulkify/context/context.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/primitives/mesh_primitive.hpp>

namespace vf {
template <typename T>
concept InstancedMeshStorage = std::convertible_to<T, std::span<DrawInstance const>>;

///
/// \brief Primitive with public GeometryBuffer, TextureHandle, and std::vector<DrawInstance> (customizable)
///
template <InstancedMeshStorage Storage = std::vector<DrawInstance>>
class InstancedMesh : public MeshPrimitive {
  public:
	static InstancedMesh makeQuad(Context const& context, std::string name, QuadCreateInfo const& info = {}, TextureHandle texture = {});

	InstancedMesh() = default;
	InstancedMesh(Context const& context, std::string name) : MeshPrimitive(context, std::move(name)) {}

	Drawable drawable() const { return {instances, gbo, texture}; }

	void draw(Surface const& surface, Pipeline const& pipeline = {}) const override { surface.draw(drawable(), pipeline); }

	Storage instances{};
};

template <typename T>
T makeQuadMesh(Context const& context, std::string name, QuadCreateInfo const& info = {}, TextureHandle texture = {});

// impl

template <InstancedMeshStorage Storage>
InstancedMesh<Storage> InstancedMesh<Storage>::makeQuad(Context const& context, std::string name, QuadCreateInfo const& info, TextureHandle texture) {
	return makeQuadMesh<InstancedMesh>(context, std::move(name), info, texture);
}

template <typename T>
T makeQuadMesh(Context const& context, std::string name, QuadCreateInfo const& info, TextureHandle texture) {
	auto ret = T(context, std::move(name));
	ret.gbo.write(Geometry::makeQuad(info));
	ret.texture = std::move(texture);
	return ret;
}
} // namespace vf
