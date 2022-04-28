#include <detail/shared_impl.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/surface.hpp>

namespace vf {
namespace {} // namespace

Drawable::Drawable(Vram const& vram, std::string name) : m_geometry(vram, std::move(name)) {}

void Drawable::draw(Surface const& surface) const {
	auto t2 = m_transform.data();
	t2.position *= 2.0f;
	InstanceData const inst[] = {{m_transform.matrix(), m_tint.normalize()}, {t2.matrix(), white_v.normalize()}};
	surface.draw(m_geometry, m_geometry.name().c_str(), inst);
}

bool Drawable::setGeometry(Geometry geometry) { return m_geometry.write(std::move(geometry)); }
} // namespace vf
