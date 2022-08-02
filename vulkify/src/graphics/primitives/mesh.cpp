#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/instanced_mesh.hpp>
#include <vulkify/graphics/primitives/mesh.hpp>

namespace vf {
Mesh Mesh::make_quad(Context const& context, std::string name, QuadCreateInfo const& info, TextureHandle texture) {
	return make_quad_mesh<Mesh>(context, std::move(name), info, texture);
}

Mesh::Mesh(Context const& context, std::string name) : MeshPrimitive(context, std::move(name)) {}

void Mesh::draw(Surface const& surface, RenderState const& state) const { surface.draw(drawable(), state); }
} // namespace vf
