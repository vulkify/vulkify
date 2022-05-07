#pragma once
#include <glm/mat3x3.hpp>
#include <vulkify/core/nvec.hpp>

namespace vf {
struct Transform {
	static glm::mat3 translation(glm::vec2 position);
	static glm::mat3 rotation(nvec2 orientation);
	static glm::mat3 scaling(glm::vec2 scale);

	glm::vec2 position{};
	nvec2 orientation{};
	glm::vec2 scale{1.0f};

	glm::mat3 matrix() const;
};

// impl

inline glm::mat3 Transform::translation(glm::vec2 position) { return glm::mat3({1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {position.x, position.y, 1.0f}); }

inline glm::mat3 Transform::rotation(nvec2 orientation) {
	auto o = orientation.value();
	return glm::mat3({o.x, o.y, 0.0f}, {-o.y, o.x, 0.0f}, {0.0f, 0.0f, 1.0f});
}

inline glm::mat3 Transform::scaling(glm::vec2 scale) { return glm::mat3({scale.x, 0.0f, 0.0f}, {0.0f, scale.y, 0.0f}, {0.0f, 0.0f, 1.0f}); }

inline glm::mat3 Transform::matrix() const { return translation(position) * rotation(orientation) * scaling(scale); }
} // namespace vf
