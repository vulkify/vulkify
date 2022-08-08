#include <vulkify/graphics/primitives/circle_shape.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
CircleShape::CircleShape(Context const& context, State initial) : Shape(context) { set_state(std::move(initial)); }

CircleShape& CircleShape::set_state(State state) {
	m_state = std::move(state);
	return refresh();
}

CircleShape& CircleShape::set_texture(Ptr<Texture const> texture, bool resize_to_match) {
	if (!texture) {
		m_texture = {};
		return *this;
	}
	m_texture = texture->handle();
	if (resize_to_match) {
		auto const extent = texture->extent();
		m_state.diameter = static_cast<float>(std::max(extent.x, extent.y));
		refresh();
	}
	return *this;
}

CircleShape& CircleShape::set_silhouette(float extrude, vf::Rgba tint) {
	if (extrude > 0.0f) {
		auto state = m_state;
		state.diameter += extrude;
		m_silhouette.buffer.write(Geometry::make_regular_polygon(state));
		m_silhouette.tint = tint;
		m_silhouette.draw = true;
	}
	return *this;
}

CircleShape& CircleShape::refresh() {
	if (m_state.diameter <= 0.0f) { return *this; }
	m_buffer.write(Geometry::make_regular_polygon(m_state));
	return *this;
}
} // namespace vf
