#include <vulkify/graphics/geometry_buffer.hpp>
#include <vulkify/graphics/primitives/all.hpp>
#include <vulkify/graphics/shader.hpp>
#include <vulkify/graphics/texture.hpp>
#include <vulkify/ttf/ttf.hpp>

namespace vf {
static_assert(std::derived_from<Texture, GfxResource>);
static_assert(std::derived_from<GeometryBuffer, GfxResource>);
static_assert(std::derived_from<Shader, GfxResource>);
static_assert(std::derived_from<Ttf, GfxResource>);
static_assert(std::derived_from<Shape, GfxResource>);
static_assert(std::derived_from<Mesh, GfxResource>);
static_assert(std::derived_from<Sprite, GfxResource>);
static_assert(std::derived_from<Text, GfxResource>);
} // namespace vf
