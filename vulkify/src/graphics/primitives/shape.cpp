#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/shape.hpp>
#include <vulkify/instance/gpu.hpp>

namespace vf {
Shape::Shape(Context const& context, std::string name) : m_buffer(context, std::move(name)) {
	m_silhouette.buffer = {context, m_buffer.name() + "_silhouette"};
}

void Shape::draw(Surface const& surface, RenderState const& state) const {
	if (m_silhouette.draw) {
		auto instance = m_instance;
		instance.tint = m_silhouette.tint;
		surface.draw(Drawable{{&instance, 1}, m_silhouette.buffer}, state);
	}
	surface.draw(Drawable{{&m_instance, 1}, m_buffer, m_texture}, state);
}
} // namespace vf
