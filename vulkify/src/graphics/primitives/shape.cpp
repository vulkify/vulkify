#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/shape.hpp>

namespace vf {
Shape::Shape(Context const& context, std::string name) : m_gbo(context.vram(), std::move(name)) {}
} // namespace vf
