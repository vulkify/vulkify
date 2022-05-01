#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/mesh.hpp>

namespace vf {
Mesh::Mesh(Context const& context, std::string name) : MeshPrimitive(context.vram(), std::move(name)) {}
} // namespace vf
