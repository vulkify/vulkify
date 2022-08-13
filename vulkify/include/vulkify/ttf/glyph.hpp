#pragma once
#include <glm/vec2.hpp>

namespace vf {
using Codepoint = std::uint32_t;

struct Glyph {
	enum struct Height : std::uint32_t { eDefault = 60 };

	static constexpr auto height_v = Height::eDefault;

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
