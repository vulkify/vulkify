#pragma once
#include <glm/vec2.hpp>

namespace vf {
template <typename T = float>
struct TopLeft : glm::tvec2<T> {
	explicit constexpr TopLeft(glm::tvec2<T> xy = {}) : glm::tvec2<T>(xy) {}
	explicit constexpr TopLeft(T x, T y) : glm::tvec2<T>(x, y) {}
	template <typename U>
	constexpr TopLeft(TopLeft<U> rhs) : TopLeft(glm::tvec2<T>(rhs)) {}
};

template <typename T = float>
struct BottomRight : glm::tvec2<T> {
	explicit constexpr BottomRight(glm::tvec2<T> xy = {}) : glm::tvec2<T>(xy) {}
	explicit constexpr BottomRight(T x, T y) : glm::tvec2<T>(x, y) {}
	template <typename U>
	constexpr BottomRight(BottomRight<U> rhs) : BottomRight(glm::tvec2<T>(rhs)) {}
};

template <typename ExtentT = float, typename OriginT = float>
struct Rect {
	glm::tvec2<ExtentT> extent{};
	glm::tvec2<OriginT> origin{};

	Rect() = default;
	constexpr Rect(glm::tvec2<ExtentT> extent, glm::tvec2<OriginT> origin = {}) : extent(extent), origin(origin) {}
	constexpr Rect(TopLeft<OriginT> topleft, BottomRight<OriginT> bottomRight);

	template <typename T, typename U>
	constexpr Rect(Rect<T, U> rhs) : Rect(rhs.extent, rhs.origin) {}

	constexpr glm::vec2 topLeft() const { return origin - glm::vec2(extent.x, -extent.y) * 0.5f; }
	constexpr glm::vec2 topRight() const { return origin + extent * 0.5f; }
	constexpr glm::vec2 bottomLeft() const { return origin - extent * 0.5f; }
	constexpr glm::vec2 bottomRight() const { return origin + glm::vec2(extent.x, -extent.y) * 0.5f; }
};

// impl

template <typename ExtentT, typename OriginT>
constexpr Rect<ExtentT, OriginT>::Rect(TopLeft<OriginT> topleft, BottomRight<OriginT> bottomRight)
	: extent(bottomRight.x - topleft.x, topleft.y - bottomRight.y), origin((topleft + bottomRight) * 0.5f) {}
} // namespace vf
