#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/mesh.hpp>

namespace vf {
Mesh::Mesh(Context const& context, std::string name) : MeshPrimitive(context, std::move(name)) {}

void Mesh::draw(Surface const& surface) const { surface.draw(drawable()); }
} // namespace vf
