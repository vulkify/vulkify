#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/circle_shape.hpp>

namespace vf {
CircleShape::CircleShape(Context const& context, std::string name, CreateInfo info) : Shape(context, std::move(name)) { setState(std::move(info)); }

CircleShape& CircleShape::setState(CircleState state) {
	m_state = std::move(state);
	return refresh();
}

CircleShape& CircleShape::setTexture(Texture texture, bool resizeToMatch) {
	m_texture = std::move(texture);
	if (resizeToMatch) {
		m_state.diameter = static_cast<float>(std::max(m_texture.extent().x, m_texture.extent().y));
		refresh();
	}
	return *this;
}

CircleShape& CircleShape::refresh() {
	if (m_state.diameter > 0.0f) { m_gbo.write(Geometry::makeRegularPolygon(m_state.diameter, m_state.points, m_state.origin, m_state.vertex)); }
	return *this;
}
} // namespace vf
