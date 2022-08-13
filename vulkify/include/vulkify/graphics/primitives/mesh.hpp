#pragma once
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/geometry_buffer.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <vulkify/graphics/surface.hpp>

namespace vf {
class Texture;

template <typename T>
concept InstancedMeshStorage = std::convertible_to<T, std::span<DrawInstance const>>;

///
/// \brief Low level primitive with public GeometryBuffer, Handle<Texture>, and std::vector<DrawInstance> (customizable)
///
template <InstancedMeshStorage Storage = std::vector<DrawInstance>>
class InstancedMesh : public Primitive {
  public:
	static InstancedMesh make_quad(GfxDevice const& device, QuadCreateInfo const& info = {}, Handle<Texture> texture = {});

	InstancedMesh() = default;
	InstancedMesh(GfxDevice const& device, Handle<Texture> texture = {}) : buffer(device), texture(texture) {}

	Drawable drawable() const { return {storage, buffer.handle(), texture}; }

	void draw(Surface const& surface, RenderState const& state = {}) const override { surface.draw(drawable(), state); }

	explicit operator bool() const { return static_cast<bool>(buffer); }

	GeometryBuffer buffer{};
	Handle<Texture> texture{};
	Storage storage{};
};

///
/// \brief Low level primitive with public GeometryBuffer, Handle<Texture>, and DrawInstance
///
class Mesh : public InstancedMesh<DrawInstance> {
  public:
	using InstancedMesh::InstancedMesh;

	DrawInstance& instance() { return storage; }
	DrawInstance const& instance() const { return storage; }
};

// impl

template <InstancedMeshStorage Storage>
InstancedMesh<Storage> InstancedMesh<Storage>::make_quad(GfxDevice const& device, QuadCreateInfo const& info, Handle<Texture> texture) {
	auto ret = InstancedMesh{device, texture};
	ret.buffer.write(Geometry::make_quad(info));
	return ret;
}
} // namespace vf
