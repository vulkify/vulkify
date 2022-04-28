#pragma once
#include <vulkify/core/nvec.hpp>
#include <vulkify/core/radian.hpp>

namespace vf {
struct Transform {
	glm::vec2 position{};
	Radian rotation{};
	glm::vec2 scale{1.0f};

	glm::mat3 matrix() const;
};

// impl

inline glm::mat3 Transform::matrix() const {
	auto const t = glm::mat3(1.0f, 0.0f, position.x, 0.0f, 1.0f, position.y, 0.0f, 0.0f, 1.0f);
	auto const cos = glm::cos(rotation);
	auto const sin = glm::sin(rotation);
	auto const r = glm::mat3(-sin, cos, 0.0f, cos, sin, 0.0f, 0.0f, 0.0f, 1.0f);
	auto const s = glm::mat3(scale.x, 0.0f, 0.0f, 0.0f, scale.y, 0.0f, 0.0f, 0.0f, 1.0f);
	return t * r * s;
}
} // namespace vf
