#pragma once
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/primitive.hpp>

namespace vf {
class MeshPrimitive : public Primitive {
  public:
	MeshPrimitive() = default;
	MeshPrimitive(Vram const& vram, std::string name) : gbo(vram, std::move(name)) {}

	GeometryBuffer gbo{};
};
} // namespace vf
