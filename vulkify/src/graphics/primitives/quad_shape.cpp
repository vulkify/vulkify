#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/quad_shape.hpp>

namespace vf {
QuadShape::QuadShape(Context const& context, std::string name, State initial) : OutlinedShape(context, std::move(name)) { setState(std::move(initial)); }

QuadShape& QuadShape::setState(State state) {
	m_state = std::move(state);
	return refresh();
}

QuadShape& QuadShape::setTexture(Texture texture, bool resizeToMatch) {
	m_texture = std::move(texture);
	if (resizeToMatch) {
		m_state.size = m_texture.extent();
		refresh();
	}
	return *this;
}

QuadShape& QuadShape::refresh() {
	if (m_state.size.x <= 0.0f || m_state.size.y <= 0.0f) { return *this; }
	m_geometry.write(Geometry::makeQuad(m_state));
	refreshOutline();
	return *this;
}
} // namespace vf
