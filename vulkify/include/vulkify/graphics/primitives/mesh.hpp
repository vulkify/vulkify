#pragma once
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <vulkify/graphics/resources/geometry_buffer.hpp>
#include <vulkify/graphics/surface.hpp>

namespace vf {
template <typename T>
concept InstancedMeshStorage = std::convertible_to<T, std::span<DrawInstance const>>;

///
/// \brief Low level primitive with public GeometryBuffer, TextureHandle, and std::vector<DrawInstance> (customizable)
///
template <InstancedMeshStorage Storage = std::vector<DrawInstance>>
class InstancedMesh : public Primitive {
  public:
	static InstancedMesh make_quad(Context const& context, std::string name, QuadCreateInfo const& info = {}, TextureHandle texture = {});

	InstancedMesh() = default;
	InstancedMesh(Context const& context, std::string name, TextureHandle texture = {}) : buffer(context, std::move(name)), texture(texture) {}

	Drawable drawable() const { return {storage, buffer, texture}; }

	void draw(Surface const& surface, RenderState const& state = {}) const override { surface.draw(drawable(), state); }

	explicit operator bool() const { return static_cast<bool>(buffer); }

	GeometryBuffer buffer{};
	TextureHandle texture{};
	Storage storage{};
};

///
/// \brief Low level primitive with public GeometryBuffer, TextureHandle, and DrawInstance
///
class Mesh : public InstancedMesh<DrawInstance> {
  public:
	using InstancedMesh::InstancedMesh;

	DrawInstance& instance() { return storage; }
	DrawInstance const& instance() const { return storage; }
};

// impl

template <InstancedMeshStorage Storage>
InstancedMesh<Storage> InstancedMesh<Storage>::make_quad(Context const& context, std::string name, QuadCreateInfo const& info, TextureHandle texture) {
	auto ret = InstancedMesh(context, std::move(name), texture);
	ret.buffer.write(Geometry::make_quad(info));
	return ret;
}
} // namespace vf
