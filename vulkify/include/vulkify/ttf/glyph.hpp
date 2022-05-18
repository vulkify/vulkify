#pragma once
#include <glm/vec2.hpp>

namespace vf {
using Codepoint = std::uint32_t;

struct Glyph {
	using Height = std::uint32_t;
	static constexpr auto height_v = Height{60};

	struct Metrics {
		glm::uvec2 extent{};
		glm::ivec2 topLeft{};
		glm::ivec2 advance{};
	};

	Metrics metrics{};
	Codepoint codepoint{};

	explicit constexpr operator bool() const { return metrics.extent.x > 0; }
};
} // namespace vf
