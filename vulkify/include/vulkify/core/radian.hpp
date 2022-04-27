#pragma once
#include <glm/trigonometric.hpp>

namespace vf {
struct Radian;

struct Degree {
	float value{};

	bool operator==(Degree const&) const = default;
	constexpr explicit operator Radian() const;
};

struct Radian {
	float value{};

	bool operator==(Radian const&) const = default;
	constexpr explicit operator Degree() const;
};

// impl

constexpr Radian::operator Degree() const { return {glm::degrees(value)}; }
constexpr Degree::operator Radian() const { return {glm::radians(value)}; }
} // namespace vf
