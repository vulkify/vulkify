#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/instanced_mesh.hpp>
#include <vulkify/graphics/primitives/mesh.hpp>

namespace vf {
Mesh Mesh::makeQuad(Context const& context, std::string name, QuadCreateInfo const& info, Texture texture) {
	return makeQuadMesh<Mesh>(context, std::move(name), info, std::move(texture));
}

Mesh::Mesh(Context const& context, std::string name) : MeshPrimitive(context, std::move(name)) {}

void Mesh::draw(Surface const& surface) const { surface.draw(drawable()); }
} // namespace vf
