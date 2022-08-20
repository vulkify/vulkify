#pragma once
#include <ktl/either.hpp>
#include <vulkify/core/nvec.hpp>
#include <vulkify/core/rect.hpp>

namespace vf {
using Extent = glm::uvec2;

///
/// \brief Whether to crop to match framebuffer aspect ratio
///
enum class Crop {
	///
	/// \brief Match larger dimension, scale smaller dimension to match aspect ratio
	///
	eFillMax,
	///
	/// \brief Match smaller dimension, scale larger dimension to match aspect ratio
	///
	eFillMin,
	///
	/// \brief Use exact extent (stretch)
	///
	eNone
};

template <Crop C>
struct LockedResize {
	glm::vec2 original{};

	constexpr glm::vec2 operator()(glm::vec2 const target) const;
};

///
/// \brief View space dynamically scaled to framebuffer or with a fixed extent
///
struct View {
	struct Size {
		Extent extent{};
		Crop crop{};
	};

	ktl::either<glm::vec2, Size> value{glm::vec2(1.0f)};

	void set_scale(glm::vec2 scale) { value = scale; }
	void set_extent(Extent extent, Crop crop = Crop::eFillMax) { value = Size{extent, crop}; }

	glm::vec2 get_scale(Extent framebuffer, glm::vec2 fallback = glm::vec2(1.0f)) const;
};

///
/// \brief Render camera
///
struct Camera {
	///
	/// \brief Render view (world space)
	///
	View view{};
	glm::vec2 position{};
	nvec2 orientation{nvec2::identity_v};
	///
	/// \brief Rendering viewport
	///
	/// origin: top-left, +x: right, +y: down, normalized [0-1]
	///
	Rect viewport{{glm::vec2(1.0f, 1.0f), glm::vec2(0.0f, 0.0f)}};
};

template <Crop C>
constexpr glm::vec2 LockedResize<C>::operator()(glm::vec2 const target) const {
	if constexpr (C == Crop::eNone) {
		return target;
	} else {
		if (FloatEq{}(original.x, {}) || FloatEq{}(original.y, {}) || FloatEq{}(target.x, {}) || FloatEq{}(target.y, {})) { return target; }
		auto const original_ar = original.x / original.y;
		auto const target_ar = target.x / target.y;
		auto match_x = [&] { return glm::vec2{target.x, target.y * target_ar / original_ar}; };
		auto match_y = [&] { return glm::vec2{target.x * original_ar / target_ar, target.y}; };
		if constexpr (C == Crop::eFillMin) {
			return target_ar < original_ar ? match_x() : match_y();
		} else {
			return target_ar < original_ar ? match_y() : match_x();
		}
	}
}
} // namespace vf
