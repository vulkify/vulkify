#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>

namespace vf {
class nvec2 {
  public:
	static constexpr float epsilon_v = 0.001f;
	static constexpr bool isEqual(float const a, float const b, float epsilon = epsilon_v) { return glm::abs(a - b) < epsilon; }

	explicit nvec2(glm::vec2 v) : m_value(glm::normalize(v)) {}
	nvec2(float x, float y) : nvec2(glm::vec2(x, y)) {}

	operator glm::vec2 const&() const { return m_value; }
	bool operator==(nvec2 const& rhs) const { return isEqual(m_value.x, rhs.m_value.x) && isEqual(m_value.y, rhs.m_value.y); }

  private:
	glm::vec2 m_value{};
};
} // namespace vf
