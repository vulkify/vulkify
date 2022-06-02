#pragma once
#include <vulkify/core/ptr.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <vulkify/graphics/resources/geometry_buffer.hpp>
#include <vulkify/graphics/resources/texture_handle.hpp>

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
