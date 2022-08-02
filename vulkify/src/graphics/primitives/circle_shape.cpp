#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/circle_shape.hpp>

namespace vf {
CircleShape::CircleShape(Context const& context, std::string name, State initial) : Shape(context, std::move(name)) { set_state(std::move(initial)); }

CircleShape& CircleShape::set_state(State state) {
	m_state = std::move(state);
	return refresh();
}

CircleShape& CircleShape::set_texture(Texture texture, bool resizeToMatch) {
	m_texture = std::move(texture);
	if (resizeToMatch) {
		m_state.diameter = static_cast<float>(std::max(m_texture.extent().x, m_texture.extent().y));
		refresh();
	}
	return *this;
}

CircleShape& CircleShape::refresh() {
	if (m_state.diameter <= 0.0f) { return *this; }
	m_geometry.write(Geometry::make_regular_polygon(m_state));
	return *this;
}
} // namespace vf
