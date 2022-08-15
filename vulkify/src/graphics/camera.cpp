#include <vulkify/graphics/camera.hpp>

namespace vf {
glm::vec2 View::get_scale(Extent framebuffer, glm::vec2 fallback) const {
	return value.visit(ktl::koverloaded{
		[fallback](glm::vec2 scale) {
			if (scale.x == 0.0f || scale.y == 0.0f) { return fallback; }
			return scale;
		},
		[framebuffer, fallback](Size const& size) {
			if (size.extent.x == 0 || size.extent.y == 0 || framebuffer.x == 0 || framebuffer.y == 0) { return fallback; }
			auto const fb = glm::vec2{framebuffer};
			auto const self = glm::vec2{size.extent};
			if (size.crop == Crop::eNone) { return fb / self; }
			auto const fb_ar = fb.x / fb.y;
			auto const self_ar = self.x / self.y;
			if (FloatEq{}(fb_ar, self_ar)) { return fb / self; }
			auto match_x = [self, fb, fb_ar] { return fb / glm::vec2{self.x, self.y / fb_ar}; };
			auto match_y = [self, fb, fb_ar] { return fb / glm::vec2{self.x * fb_ar, self.y}; };
			if (size.crop == Crop::eFillMin) { return fb_ar > self_ar ? match_x() : match_y(); }
			return fb_ar > self_ar ? match_y() : match_x();
		},
	});
}
} // namespace vf
