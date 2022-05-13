#pragma once
#include <vulkify/core/rect.hpp>

namespace vf {
using Codepoint = std::uint32_t;

struct Glyph {
	struct Metrics {
		glm::uvec2 extent{};
		glm::ivec2 topLeft{};
		glm::ivec2 advance{};
	};

	Metrics metrics{};
	UVRect uv{};
	Codepoint codepoint{};
};
} // namespace vf
