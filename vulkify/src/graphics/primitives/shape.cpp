#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/shape.hpp>

namespace vf {
Shape::Shape(Context const& context, std::string name) : m_gbo(context, std::move(name)) {}

void Shape::draw(Surface const& surface) const { surface.draw(drawable()); }
} // namespace vf
