#include <detail/shared_impl.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/surface.hpp>

namespace vf {
Drawable::Drawable(Vram const& vram, std::string name) : m_geometry(vram, std::move(name)) {}

void Drawable::draw(Surface const& surface) const {
	auto t2 = m_instance.transform;
	t2.position += glm::vec2(200.0f, 0.0f);
	auto m = m_instance.model();
	DrawModel const models[] = {m, DrawInstance{t2, white_v}.model()};
	surface.draw(m_geometry, m_geometry.name().c_str(), models);
}

bool Drawable::setGeometry(Geometry geometry) { return m_geometry.write(std::move(geometry)); }
} // namespace vf
