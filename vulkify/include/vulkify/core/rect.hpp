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
/// \brief World-space 2D rect
///
/// origin: centre, +x: right, +y: up
///
struct Rect : TRect<float> {
	constexpr glm::vec2 top_left() const { return offset - glm::vec2(extent.x, -extent.y) * 0.5f; }
	constexpr glm::vec2 top_right() const { return offset + extent * 0.5f; }
	constexpr glm::vec2 bottom_left() const { return offset - extent * 0.5f; }
	constexpr glm::vec2 bottom_right() const { return offset + glm::vec2(extent.x, -extent.y) * 0.5f; }

	constexpr bool contains(glm::vec2 point) const {
		auto const in = [](float t, float min, float max) { return t >= min && t <= max; };
		auto const bl = bottom_left();
		auto const tr = top_right();
		return in(point.x, bl.x, tr.x) && in(point.y, bl.y, tr.y);
	}

	constexpr bool intersects(Rect const& other) const {
		auto const in = [](Rect const& a, Rect const& b) {
			return a.contains(b.top_left()) || a.contains(b.top_right()) || a.contains(b.bottom_left()) || a.contains(b.bottom_right());
		};
		return in(*this, other) || in(other, *this);
	}
};

///
/// \brief Normalized texture coordinates for a quad
///
/// origin: top-left, +x: right, +y: down, normalized [0-1]
///
struct UvRect {
	glm::vec2 top_left{0.0f, 0.0f};
	glm::vec2 bottom_right{1.0f, 1.0f};
};
} // namespace vf
