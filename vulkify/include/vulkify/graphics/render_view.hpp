#pragma once
#include <vulkify/core/nvec.hpp>
#include <vulkify/core/rect.hpp>

namespace vf {
///
/// \brief Rendering viewport
///
/// origin: top-left, +x: right, +y: down, normalized [0-1]
///
struct Viewport : TRect<float> {
	Viewport() = default;
	constexpr Viewport(glm::vec2 extent, glm::vec2 topLeft) {
		this->extent = extent;
		offset = topLeft;
	}

	constexpr Viewport& operator*=(glm::vec2 scale);
};

constexpr Viewport operator*(Viewport const& a, glm::vec2 scale);
constexpr Viewport operator*(glm::vec2 scale, Viewport const& a);

///
/// \brief Default viewport
///
constexpr auto viewport_v = Viewport{{1.0f, 1.0f}, {0.0f, 0.0f}};

///
/// \brief Render view (camera)
///
struct RenderView {
	glm::vec2 position{};
	nvec2 orientation{nvec2::identity_v};
	Viewport viewport{viewport_v};
};

// impl

constexpr Viewport& Viewport::operator*=(glm::vec2 const scale) {
	this->extent *= scale;
	this->offset *= scale;
	return *this;
}

constexpr Viewport operator*(Viewport const& a, glm::vec2 const scale) {
	auto ret = a;
	ret *= scale;
	return ret;
}

constexpr Viewport operator*(glm::vec2 const scale, Viewport const& a) { return a * scale; }
} // namespace vf
