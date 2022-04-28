#include <detail/shared_impl.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/surface.hpp>

namespace vf {
namespace {
DrawModel makeModel(Transform const& tr, Rgba const tint) {
	auto const pos_rot = glm::vec4(tr.position, tr.rotation, 0.0f);
	return {pos_rot, glm::vec4(tr.scale, 0.0f, 0.0f), tint.normalize()};
}
} // namespace

DrawModel DrawInstance::model() const { return makeModel(transform, tint); }

Drawable::Drawable(Vram const& vram, std::string name) : m_geometry(vram, std::move(name)) {}

void Drawable::draw(Surface const& surface) const {
	auto t2 = m_instance.transform;
	t2.position += glm::vec2(200.0f, 0.0f);
	DrawModel const models[] = {m_instance.model(), makeModel(t2, white_v)};
	surface.draw(m_geometry, m_geometry.name().c_str(), models);
}

bool Drawable::setGeometry(Geometry geometry) { return m_geometry.write(std::move(geometry)); }
} // namespace vf
