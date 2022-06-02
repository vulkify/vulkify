#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/shape.hpp>
#include <vulkify/instance/gpu.hpp>

namespace vf {
Shape::Shape(Context const& context, std::string name) : m_geometry(context, std::move(name)) {}

void Shape::Silhouette::draw(Shape const& shape, Surface const& surface, RenderState const& state) const {
	auto tr = shape.transform();
	tr.scale *= scale;
	auto const instance = DrawInstance{tr, tint};
	surface.draw(Drawable{{&instance, 1}, shape.geometry()}, state);
}

void Shape::draw(Surface const& surface, RenderState const& state) const {
	if (silhouette.scale > 0.0f) { silhouette.draw(*this, surface, state); }
	surface.draw(Drawable{{&m_instance, 1}, m_geometry, m_texture.handle()}, state);
}
} // namespace vf
