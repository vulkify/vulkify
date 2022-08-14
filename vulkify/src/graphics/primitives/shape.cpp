#include <vulkify/graphics/primitives/shape.hpp>
#include <vulkify/graphics/surface.hpp>
#include <vulkify/instance/gpu.hpp>

namespace vf {
Shape::Shape(GfxDevice const& device) : Prop(device) { m_silhouette.buffer = GeometryBuffer{device}; }

void Shape::draw(Surface const& surface, RenderState const& state) const {
	if (m_silhouette.draw) {
		auto instance = m_instance;
		instance.tint = m_silhouette.tint;
		surface.draw(Drawable{instance, m_silhouette.buffer.handle()}, state);
	}
	surface.draw(Drawable{m_instance, m_buffer.handle(), m_texture}, state);
}
} // namespace vf
