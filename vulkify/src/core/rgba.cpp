#include <glm/glm.hpp>
#include <glm/gtc/color_space.hpp>
#include <vulkify/core/rgba.hpp>
#include <algorithm>

namespace vf {
float Rgba::normalize(Channel ch) { return std::clamp(static_cast<float>(ch) / static_cast<float>(0xff), 0.0f, 1.0f); }
Rgba::Channel Rgba::unnormalize(float f) { return static_cast<Channel>(std::clamp(f, 0.0f, 1.0f) * static_cast<float>(0xff)); }

glm::vec4 Rgba::srgb(glm::vec4 const& linear) { return glm::convertLinearToSRGB(linear); }
glm::vec4 Rgba::linear(glm::vec4 const& srgb) { return glm::convertSRGBToLinear(srgb); }

Rgba Rgba::srgb() const { return make(srgb(normalize())); }
Rgba Rgba::linear() const { return make(linear(normalize())); }

Rgba Rgba::lerp(Rgba const a, Rgba const b, float const t) {
	auto lc = [&](int i) { return unnormalize(normalize(a.channels[i]) * (1.0f - t) + normalize(b.channels[i]) * t); };
	auto ret = Rgba{};
	for (int i = 0; i < 4; ++i) { ret.channels[i] = lc(i); }
	return ret;
}
} // namespace vf
