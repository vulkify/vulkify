#include <ktl/fixed_vector.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <vulkify/graphics/surface.hpp>
#include <vulkify/graphics/texture.hpp>
#include <iterator>

namespace vf {
namespace {
template <typename T>
void drawPrimitive(Surface const& surface, Primitive const& primitive, T& out) {
	static auto const s_blank = Texture{};
	Primitive::addDrawModels(primitive.instances, std::back_inserter(out));
	surface.draw(*primitive.vbo, out, primitive.texture ? *primitive.texture : s_blank);
}
} // namespace

void Primitive::draw(Surface const& surface) const {
	if (instances.size() <= small_buffer_v) {
		auto buffer = ktl::fixed_vector<DrawModel, small_buffer_v>{};
		drawPrimitive(surface, *this, buffer);
	} else {
		auto buffer = std::vector<DrawModel>{};
		buffer.reserve(instances.size());
		drawPrimitive(surface, *this, buffer);
	}
}
} // namespace vf
