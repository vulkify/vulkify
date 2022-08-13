#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/shape.hpp>
#include <vulkify/instance/gpu.hpp>

namespace vf {
Shape::Shape(Context const& context) : Prop(context) { m_silhouette.buffer = refactor::GeometryBuffer{context}; }

void Shape::draw(refactor::Surface const& surface, RenderState const& state) const {
	if (m_silhouette.draw) {
		auto instance = m_instance;
		instance.tint = m_silhouette.tint;
		surface.draw(refactor::Drawable{instance, m_silhouette.buffer.handle()}, state);
	}
	surface.draw(refactor::Drawable{m_instance, m_buffer.handle(), m_texture}, state);
}
} // namespace vf
