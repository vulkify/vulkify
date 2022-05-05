#include <vulkify/context/context.hpp>
#include <vulkify/graphics/primitives/shape.hpp>

namespace vf {
Shape::Shape(Context const& context, std::string name) : m_geometry(context, std::move(name)) {}

void Shape::draw(Surface const& surface) const { surface.draw(Drawable{{&m_instance, 1}, m_geometry, m_texture}); }

OutlinedShape::OutlinedShape(Context const& context, std::string name) : Shape(context, std::move(name)), m_outline(context, m_geometry.name() + "_outline") {
	m_outline.state.topology = Topology::eLineStrip;
	m_outline.state.polygonMode = PolygonMode::eLine;
}

void OutlinedShape::setOutline(float lineWidth, Rgba rgba) {
	bool const rgbaMatch = rgba == outlineRgba();
	if (rgbaMatch && FloatEq{}(lineWidth, outlineWidth())) { return; }
	auto geometry = m_geometry.geometry();
	if (geometry.vertices.empty()) { return; }

	if (!rgbaMatch) { buildOutline(std::move(geometry), rgba); }

	m_outline.state.lineWidth = lineWidth;
}

void OutlinedShape::draw(Surface const& surface) const {
	Shape::draw(surface);
	auto const outlineInstance = DrawInstance{m_instance.transform};
	surface.draw(Drawable{{&outlineInstance, 1}, m_outline, {}});
}

void OutlinedShape::buildOutline(Geometry geometry, Rgba rgba) {
	auto const rgbav = rgba.normalize();
	for (auto& vertex : geometry.vertices) {
		vertex.uv = {};
		vertex.rgba = rgbav;
	}

	geometry.indices.clear();
	geometry.indices.reserve(geometry.vertices.size() + 1);
	for (std::uint32_t i = 0; i < geometry.vertices.size(); ++i) { geometry.indices.push_back(i); }
	geometry.indices.push_back(0);

	m_outline.write(std::move(geometry));
	m_outlineRgba = rgba;
}
} // namespace vf
