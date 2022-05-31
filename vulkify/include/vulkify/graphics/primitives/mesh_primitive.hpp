#pragma once
#include <vulkify/core/ptr.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/handles.hpp>
#include <vulkify/graphics/primitive.hpp>

namespace vf {
///
/// \brief Abstract Primitive owning GeometryBuffer and pointer to const Texture
///
class MeshPrimitive : public Primitive {
  public:
	MeshPrimitive() = default;
	MeshPrimitive(Context const& context, std::string name, TextureHandle texture = {}) : gbo(context, std::move(name)), texture(texture) {}

	explicit operator bool() const { return static_cast<bool>(gbo); }

	GeometryBuffer gbo{};
	TextureHandle texture{};
};
} // namespace vf
