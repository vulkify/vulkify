#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/circle_shape.hpp>

namespace vf {
CircleShape::CircleShape(Context const& context, std::string name, State initial) : OutlinedShape(context, std::move(name)) { setState(std::move(initial)); }

CircleShape& CircleShape::setState(State state) {
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

void CircleShape::refreshOutline() {
	auto geometry = m_geometry.geometry();
	if (geometry.vertices.empty()) { return; }
	// remove centre
	geometry.vertices.erase(geometry.vertices.begin());
	writeOutline(std::move(geometry));
}

CircleShape& CircleShape::refresh() {
	if (m_state.diameter <= 0.0f) { return *this; }
	m_geometry.write(Geometry::makeRegularPolygon(m_state));
	refreshOutline();
	return *this;
}
} // namespace vf
