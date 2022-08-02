#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/quad_shape.hpp>

namespace vf {
QuadShape::QuadShape(Context const& context, std::string name, State initial) : Shape(context, std::move(name)) { set_state(std::move(initial)); }

QuadShape& QuadShape::set_state(State state) {
	m_state = std::move(state);
	return refresh();
}

QuadShape& QuadShape::set_texture(Texture texture, bool resizeToMatch) {
	m_texture = std::move(texture);
	if (resizeToMatch) {
		m_state.size = m_texture.extent();
		refresh();
	}
	return *this;
}

QuadShape& QuadShape::refresh() {
	if (m_state.size.x <= 0.0f || m_state.size.y <= 0.0f) { return *this; }
	m_geometry.write(Geometry::make_quad(m_state));
	return *this;
}
} // namespace vf
