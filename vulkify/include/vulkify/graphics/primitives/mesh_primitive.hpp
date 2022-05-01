#pragma once
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/primitive.hpp>

namespace vf {
///
/// \brief Abstract Primitive owning GeometryBuffer
///
/// Note: End-user types should take a (const reference to) Context and pass Vram from it in their constructors.
/// Context can be forward declared in the header, as the user will need an instance to pass its reference.
///
class MeshPrimitive : public Primitive {
  public:
	MeshPrimitive() = default;
	MeshPrimitive(Vram const& vram, std::string name) : gbo(vram, std::move(name)) {}

	GeometryBuffer gbo{};
};
} // namespace vf
