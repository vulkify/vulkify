#include <detail/shared_impl.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/graphics/draw_object.hpp>
#include <vulkify/graphics/surface.hpp>

namespace vf {
namespace {
template <typename T>
void drawModels(Surface const& surface, GeometryBuffer const& geometry, std::span<DrawInstance const> instances, T& out, Texture const* tex) {
	DrawInstance::addModels(instances, std::back_inserter(out));
	surface.draw(geometry, geometry.name().c_str(), out, tex);
}
} // namespace

void DrawObject::draw(Surface const& surface) const {
	auto const& geometry = m_model->geometry();
	auto const& instances = m_model->instances();
	if (!geometry || instances.empty()) { return; }
	if (instances.size() <= small_buffer_v) {
		auto models = ktl::fixed_vector<DrawModel, small_buffer_v>{};
		drawModels(surface, geometry, instances, models, m_model->texture());
	} else {
		auto models = std::vector<DrawModel>{};
		drawModels(surface, geometry, instances, models, m_model->texture());
	}
}
} // namespace vf
