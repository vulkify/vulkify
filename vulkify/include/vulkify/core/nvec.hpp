#pragma once
#include <glm/vec2.hpp>
#include <vulkify/core/radian.hpp>
#include <cmath>

namespace vf {
class nvec2 {
  public:
	static constexpr float epsilon_v = 0.001f;
	static constexpr bool isEqual(float const a, float const b, float epsilon = epsilon_v) { return std::abs(a - b) < epsilon; }

	static constexpr float sqrMag(glm::vec2 in);
	static constexpr glm::vec2 normalize(glm::vec2 in);

	nvec2() = default;
	explicit constexpr nvec2(glm::vec2 v) : m_value(normalize(v)) {}
	constexpr nvec2(float x, float y) : nvec2(glm::vec2(x, y)) {}

	static constexpr glm::vec2 rotate(glm::vec2 vec, Radian rad);

	constexpr nvec2& rotate(Radian rad);

	constexpr operator glm::vec2 const&() const { return m_value; }
	constexpr bool operator==(nvec2 const& rhs) const { return isEqual(m_value.x, rhs.m_value.x) && isEqual(m_value.y, rhs.m_value.y); }

  private:
	glm::vec2 m_value{1.0f, 0.0f};
};

// impl

constexpr float nvec2::sqrMag(glm::vec2 const in) { return in.x * in.x + in.y * in.y; }

constexpr glm::vec2 nvec2::normalize(glm::vec2 const in) {
	auto const mag = std::sqrt(sqrMag(in));
	return isEqual(mag, epsilon_v) ? glm::vec2{} : in / mag;
}

constexpr glm::vec2 nvec2::rotate(glm::vec2 const vec, Radian const rad) {
	auto const cos = std::cos(rad);
	auto const sin = std::sin(rad);
	return {vec.x * cos - vec.y * sin, vec.x * sin + vec.y * cos};
}

constexpr nvec2& nvec2::rotate(Radian rad) {
	m_value = rotate(m_value, rad);
	return *this;
}
} // namespace vf
