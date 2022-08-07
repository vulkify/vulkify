#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/shape.hpp>
#include <vulkify/instance/gpu.hpp>

namespace vf {
Shape::Shape(Context const& context) : Prop(context) { m_silhouette.buffer = GeometryBuffer{context}; }

void Shape::draw(Surface const& surface, RenderState const& state) const {
	if (m_silhouette.draw) {
		auto instance = m_instance;
		instance.tint = m_silhouette.tint;
		surface.draw(Drawable{instance, m_silhouette.buffer}, state);
	}
	surface.draw(Drawable{m_instance, m_buffer, m_texture}, state);
}
} // namespace vf
