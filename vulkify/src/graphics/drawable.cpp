#include <detail/shared_impl.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/surface.hpp>

namespace vf {
void Drawer::operator()(Surface const& surface, GeometryBuffer const& geometry) const {
	drawParams.modelMatrix = transform.matrix();
	surface.draw(geometry, drawParams);
}

Drawable::Drawable(Vram const& vram, std::string name) : m_geometry(vram, std::move(name)) {}

void Drawable::draw(Surface const& surface) const { m_drawer(surface, m_geometry); }

bool Drawable::setGeometry(Geometry geometry) { return m_geometry.write(std::move(geometry)); }
} // namespace vf
