#include <vulkify/graphics/primitives/quad_shape.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
QuadShape::QuadShape(GfxDevice const& device, State initial) : Shape(device) { set_state(std::move(initial)); }

QuadShape& QuadShape::set_state(State state) {
	m_state = std::move(state);
	return refresh();
}

QuadShape& QuadShape::set_texture(Ptr<Texture const> texture, bool resize_to_match) {
	if (!texture) {
		m_texture = {};
		return *this;
	}
	m_texture = texture->handle();
	if (resize_to_match) {
		m_state.size = texture->extent();
		refresh();
	}
	return *this;
}

QuadShape& QuadShape::set_silhouette(float extrude, Rgba tint) {
	if (extrude > 0.0f) {
		auto state = m_state;
		state.size += extrude;
		m_silhouette.buffer.write(Geometry::make_quad(state));
		m_silhouette.tint = tint;
		m_silhouette.draw = true;
	}
	return *this;
}

QuadShape& QuadShape::refresh() {
	if (m_state.size.x <= 0.0f || m_state.size.y <= 0.0f) { return *this; }
	m_buffer.write(Geometry::make_quad(m_state));
	return *this;
}
} // namespace vf
