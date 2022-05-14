#pragma once
#include <glm/vec2.hpp>

namespace vf {
///
/// \brief Generic 2D rect; origin and axis directions may vary across usage
///
template <typename Type>
struct TRect {
	glm::tvec2<Type> extent{};
	glm::tvec2<Type> offset{};

	template <typename T>
	constexpr operator TRect<T>() const {
		return {glm::tvec2<T>(extent), glm::tvec2<T>(offset)};
	}
};

///
/// \brief Data structure specifying world-space 2D rect
///
/// origin: centre, +x: right, +y: up
///
struct Rect : TRect<float> {
	constexpr glm::vec2 topLeft() const { return offset - glm::vec2(extent.x, -extent.y) * 0.5f; }
	constexpr glm::vec2 topRight() const { return offset + extent * 0.5f; }
	constexpr glm::vec2 bottomLeft() const { return offset - extent * 0.5f; }
	constexpr glm::vec2 bottomRight() const { return offset + glm::vec2(extent.x, -extent.y) * 0.5f; }
};

///
/// \brief Data structure specifying a quad's texture coordinates
///
/// origin: top-left, +x: right, +y: down, normalized [0-1]
///
struct UVRect {
	glm::vec2 topLeft{0.0f, 0.0f};
	glm::vec2 bottomRight{1.0f, 1.0f};
};
} // namespace vf
