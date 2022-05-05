#pragma once
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/primitive.hpp>

namespace vf {
///
/// \brief Abstract Primitive owning GeometryBuffer
///
class MeshPrimitive : public Primitive {
  public:
	MeshPrimitive() = default;
	MeshPrimitive(Context const& context, std::string name) : gbo(context, std::move(name)) {}

	GeometryBuffer gbo{};
};
} // namespace vf
