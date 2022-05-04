#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/quad_shape.hpp>

namespace vf {
QuadShape::QuadShape(Context const& context, std::string name, CreateInfo info) : Shape(context, std::move(name)) { setState(std::move(info)); }

QuadShape& QuadShape::setState(QuadState state) {
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
	if (m_state.size.x > 0.0f && m_state.size.y > 0.0f) { m_gbo.write(Geometry::makeQuad(m_state.size, m_state.origin, m_state.vertex, m_state.uv)); }
	return *this;
}
} // namespace vf
