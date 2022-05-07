#pragma once
#include <glm/vec2.hpp>

namespace vf {
template <typename ExtentT = float, typename OriginT = float>
struct TRect {
	glm::tvec2<ExtentT> extent{};
	glm::tvec2<OriginT> origin{};

	static constexpr TRect make(glm::tvec2<OriginT> topleft, glm::tvec2<OriginT> bottomRight);

	constexpr glm::vec2 topLeft() const { return origin - glm::vec2(extent.x, -extent.y) * 0.5f; }
	constexpr glm::vec2 topRight() const { return origin + extent * 0.5f; }
	constexpr glm::vec2 bottomLeft() const { return origin - extent * 0.5f; }
	constexpr glm::vec2 bottomRight() const { return origin + glm::vec2(extent.x, -extent.y) * 0.5f; }

	template <typename T, typename U>
	constexpr operator TRect<T, U>() const;
};

using Rect = TRect<>;
constexpr auto viewport_v = Rect{{1.0f, 1.0f}, {0.0f, 0.0f}};

// impl

template <typename ExtentT, typename OriginT>
constexpr TRect<ExtentT, OriginT> TRect<ExtentT, OriginT>::make(glm::tvec2<OriginT> topleft, glm::tvec2<OriginT> bottomRight) {
	return {{bottomRight.x - topleft.x, topleft.y - bottomRight.y}, glm::vec2(topleft + bottomRight) * 0.5f};
}

template <typename ExtentT, typename OriginT>
template <typename T, typename U>
constexpr TRect<ExtentT, OriginT>::operator TRect<T, U>() const {
	return {glm::tvec2<T>(extent), glm::tvec2<U>(origin)};
}
} // namespace vf
