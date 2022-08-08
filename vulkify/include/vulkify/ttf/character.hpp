#pragma once
#include <vulkify/core/ptr.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/ttf/glyph.hpp>

namespace vf {
struct Character {
	Ptr<Glyph const> glyph{};
	UvRect uv{};

	explicit operator bool() const { return glyph && *glyph; }
};
} // namespace vf
