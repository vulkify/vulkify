#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/shape.hpp>
#include <vulkify/instance/gpu.hpp>

namespace vf {
Shape::Shape(Context const& context, std::string name) : m_geometry(context, std::move(name)) {
	m_silhouette.gbo = {context, m_geometry.name() + "_silhouette"};
}

void Shape::draw(Surface const& surface, RenderState const& state) const {
	if (m_silhouette.draw) {
		auto instance = m_instance;
		instance.tint = m_silhouette.tint;
		surface.draw(Drawable{{&instance, 1}, m_silhouette.gbo}, state);
	}
	surface.draw(Drawable{{&m_instance, 1}, m_geometry, m_texture}, state);
}
} // namespace vf
