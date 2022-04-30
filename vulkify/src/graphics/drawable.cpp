#include <detail/shared_impl.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/surface.hpp>

namespace vf {
namespace {
template <typename T>
void drawModels(Surface const& surface, GeometryBuffer const& geometry, std::span<Primitive::Instance const> instances, T& out, Texture const* tex) {
	Primitive::addDrawModels(instances, std::back_inserter(out));
	surface.draw(geometry, out, tex);
}
} // namespace

void Drawable::draw(Surface const& surface) const {
	auto const primitive = m_model->primitive();
	if (!primitive) { return; }
	if (primitive.instances.size() <= small_buffer_v) {
		auto models = ktl::fixed_vector<DrawModel, small_buffer_v>{};
		drawModels(surface, *primitive.vbo, primitive.instances, models, primitive.texture);
	} else {
		auto models = std::vector<DrawModel>{};
		drawModels(surface, *primitive.vbo, primitive.instances, models, primitive.texture);
	}
}
} // namespace vf
