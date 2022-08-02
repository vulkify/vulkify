#pragma once
#include <ktl/either.hpp>
#include <vulkify/core/nvec.hpp>
#include <vulkify/core/rect.hpp>

namespace vf {
using Extent = glm::uvec2;

///
/// \brief View space dynamically scaled to framebuffer or with a fixed extent
///
/// If set as glm::vec2, will be interpreted as a relative scale,
/// otherwise as absolute extent
///
struct View {
	ktl::either<glm::vec2, Extent> value{glm::vec2(1.0f)};

	void set_scale(glm::vec2 scale) { value = scale; }
	void set_extent(Extent extent) { value = extent; }

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

// impl

inline glm::vec2 View::get_scale(Extent framebuffer, glm::vec2 fallback) const {
	return value.visit(ktl::koverloaded{
		[fallback](glm::vec2 scale) {
			if (scale.x == 0.0f || scale.y == 0.0f) { return fallback; }
			return scale;
		},
		[framebuffer, fallback](Extent e) {
			if (e.x == 0 || e.y == 0) { return fallback; }
			return glm::vec2(framebuffer) / glm::vec2(e);
		},
	});
}
} // namespace vf
