#pragma once
#include <vulkify/graphics/image.hpp>

namespace vf {
struct Icon {
	using TopLeft = glm::ivec2;

	Image::View bitmap{};
	TopLeft hotspot{};
};
} // namespace vf
