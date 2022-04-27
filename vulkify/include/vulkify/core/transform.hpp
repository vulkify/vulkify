#pragma once
#include <glm/ext/matrix_transform.hpp>
#include <vulkify/core/nvec.hpp>
#include <vulkify/core/radian.hpp>

namespace vf {
class Transform {
  public:
	struct Data {
		glm::vec2 position{};
		nvec2 orientation{0.0f, 1.0f};
		glm::vec2 scale{1.0f};

		glm::mat3 matrix() const;
	};

	Data const& data() const { return m_data; }

	glm::vec2 position() const { return m_data.position; }
	nvec2 orientation() const { return m_data.orientation; }
	glm::vec2 scale() const { return m_data.scale; }
	glm::mat3 const& matrix() const { return m_dirty ? refresh() : m_matrix; }

	Transform& setPosition(glm::vec2 xy);
	Transform& setOrientation(nvec2 xy);
	Transform& setScale(nvec2 xy);

	Transform& setDirty();
	glm::mat3 const& refresh() const;

  private:
	Data m_data{};
	mutable glm::mat3 m_matrix{};
	mutable bool m_dirty{};
};

// impl

inline glm::mat3 Transform::Data::matrix() const {
	auto const t = glm::mat3(glm::vec3(1.0f, 0.0f, position.x), glm::vec3(0.0f, 1.0f, position.y), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::vec2 const& o = orientation;
	auto const r = glm::mat3(glm::vec3(o.y, -o.x, 0.0f), glm::vec3(o.x, o.y, 0.0f), glm::vec3(0.0, 0.0, 1.0f));
	auto const s = glm::mat3(glm::vec3(scale.x, 0.0f, 0.0f), glm::vec3(0.0f, scale.y, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	auto ret = t * r * s;
	return ret;
}

inline Transform& Transform::setPosition(glm::vec2 const xy) {
	m_data.position = xy;
	return setDirty();
}

inline Transform& Transform::setOrientation(nvec2 const xy) {
	m_data.orientation = xy;
	return setDirty();
}

inline Transform& Transform::setScale(nvec2 const xy) {
	m_data.scale = xy;
	return setDirty();
}

inline Transform& Transform::setDirty() {
	m_dirty = true;
	return *this;
}

inline glm::mat3 const& Transform::refresh() const {
	m_dirty = false;
	return m_matrix = m_data.matrix();
}
} // namespace vf
