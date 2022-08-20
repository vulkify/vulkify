#pragma once
#include <glm/vec2.hpp>

namespace vf {
using Codepoint = std::uint32_t;

struct Glyph {
	enum struct Height : std::uint32_t { eDefault = 60 };

	static constexpr auto height_v = Height::eDefault;

	struct Metrics {
		glm::uvec2 extent{};
		glm::ivec2 top_left{};
		glm::ivec2 advance{};
	};

	Metrics metrics{};
	Codepoint codepoint{};

	explicit constexpr operator bool() const { return metrics.extent.x > 0; }
};

struct CharSize {
	static constexpr auto scaling_v{2.0f};

	Glyph::Height height{Glyph::height_v};
	float scaling{scaling_v};

	static constexpr float safe_scaling(float const scaling, float const fallback = 1.0f) { return scaling <= 0.0f ? fallback : scaling; }

	constexpr Glyph::Height glyph_height() const { return static_cast<Glyph::Height>(static_cast<float>(height) * safe_scaling(scaling)); }
	constexpr float quad_scale() const { return 1.0f / safe_scaling(scaling); }
};
} // namespace vf
