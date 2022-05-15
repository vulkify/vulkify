#pragma once
#include <vulkify/core/ptr.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/primitive.hpp>

namespace vf {
class Texture;

///
/// \brief Abstract Primitive owning GeometryBuffer and pointer to const Texture
///
class MeshPrimitive : public Primitive {
  public:
	MeshPrimitive() = default;
	MeshPrimitive(Context const& context, std::string name, Ptr<Texture> texture = {}) : gbo(context, std::move(name)), texture(texture) {}

	explicit operator bool() const { return static_cast<bool>(gbo); }

	GeometryBuffer gbo{};
	Ptr<Texture const> texture{};
};
} // namespace vf
