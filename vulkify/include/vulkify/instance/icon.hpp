#pragma once
#include <vulkify/graphics/bitmap.hpp>

namespace vf {
struct Icon {
	using TopLeft = glm::ivec2;

	Bitmap::View bitmap{};
	TopLeft hotspot{};
};
} // namespace vf
