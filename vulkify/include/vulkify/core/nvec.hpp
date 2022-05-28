#pragma once
#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <vulkify/core/float_eq.hpp>
#include <vulkify/core/radian.hpp>
#include <cmath>

namespace vf {
///
/// \brief Normalized 2D vector
///
/// Identity faces right (+1, 0)
///
class nvec2 {
  public:
	static nvec2 const identity_v;
	static nvec2 const right_v;
	static nvec2 const down_v;
	static nvec2 const left_v;
	static nvec2 const up_v;

	static constexpr float sqrMag(glm::vec2 in);
	static glm::vec2 normalize(glm::vec2 in);

	nvec2() = default;
	explicit nvec2(glm::vec2 v) : m_value(normalize(v)) {}
	explicit nvec2(float x, float y) : nvec2(glm::vec2(x, y)) {}
	explicit nvec2(Radian rad) : m_value(glm::cos(rad), glm::sin(rad)) {}

	static glm::vec2 rotate(glm::vec2 vec, Radian rad);
	static float dot(nvec2 a, nvec2 b) { return glm::dot(a.m_value, b.m_value); }

	nvec2& rotate(Radian rad);
	nvec2& invert();
	nvec2 rotated(Radian rad) const;
	nvec2 inverted() const;

	Radian radian() const;

	constexpr glm::vec2 const& value() const { return m_value; }
	constexpr operator glm::vec2 const&() const { return value(); }
	constexpr bool operator==(nvec2 const& rhs) const { return FloatEq{}(m_value.x, rhs.m_value.x) && FloatEq{}(m_value.y, rhs.m_value.y); }

  private:
	glm::vec2 m_value{1.0f, 0.0f};
};

// impl

inline nvec2 const nvec2::identity_v = nvec2{};
inline nvec2 const nvec2::right_v = identity_v;
inline nvec2 const nvec2::down_v = nvec2{0.0f, -1.0f};
inline nvec2 const nvec2::left_v = nvec2{-1.0f, 0.0f};
inline nvec2 const nvec2::up_v = nvec2{0.0f, 1.0f};

constexpr float nvec2::sqrMag(glm::vec2 const in) { return in.x * in.x + in.y * in.y; }

inline glm::vec2 nvec2::normalize(glm::vec2 const in) {
	auto const mag = std::sqrt(sqrMag(in));
	return FloatEq{}(mag, 0.0f) ? glm::vec2{} : in / mag;
}

inline glm::vec2 nvec2::rotate(glm::vec2 const vec, Radian const rad) {
	auto const cos = std::cos(rad);
	auto const sin = std::sin(rad);
	return {vec.x * cos - vec.y * sin, vec.x * sin + vec.y * cos};
}

inline nvec2& nvec2::rotate(Radian rad) {
	m_value = rotate(m_value, rad);
	return *this;
}

inline nvec2& nvec2::invert() {
	m_value.y = -m_value.y;
	return *this;
}

inline nvec2 nvec2::rotated(Radian rad) const {
	auto ret = *this;
	ret.rotate(rad);
	return ret;
}

inline nvec2 nvec2::inverted() const {
	auto ret = *this;
	ret.invert();
	return ret;
}

inline Radian nvec2::radian() const {
	auto ret = std::acos(m_value.x);
	return m_value.y < 0.0f ? Radian{-ret} : Radian{ret};
}
} // namespace vf
