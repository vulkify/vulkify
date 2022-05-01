#include <vulkify/graphics/primitive.hpp>
#include <vulkify/graphics/surface.hpp>

namespace vf {
void Primitive::draw(Surface const& surface) const { surface.draw(drawable()); }
} // namespace vf
