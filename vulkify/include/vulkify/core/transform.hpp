#pragma once
#include <glm/mat3x3.hpp>
#include <vulkify/core/nvec.hpp>

namespace vf {
struct Transform {
	glm::vec2 position{};
	nvec2 orientation{};
	glm::vec2 scale{1.0f};

	glm::mat3 matrix() const;
};

// impl

inline glm::mat3 Transform::matrix() const {
	auto const t = glm::mat3(1.0f, 0.0f, position.x, 0.0f, 1.0f, position.y, 0.0f, 0.0f, 1.0f);
	auto orn = static_cast<glm::vec2 const&>(orientation);
	auto const r = glm::mat3(orn.x, -orn.y, 0.0f, orn.y, orn.x, 0.0f, 0.0f, 0.0f, 1.0f);
	auto const s = glm::mat3(scale.x, 0.0f, 0.0f, 0.0f, scale.y, 0.0f, 0.0f, 0.0f, 1.0f);
	return t * r * s;
}
} // namespace vf
