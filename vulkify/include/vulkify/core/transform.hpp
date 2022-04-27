#pragma once
#include <glm/gtx/rotate_vector.hpp>
#include <vulkify/core/nvec.hpp>
#include <vulkify/core/radian.hpp>

namespace vf {
class Transform {
  public:
	struct Data {
		glm::vec2 position{};
		Radian rotation{};
		glm::vec2 scale{1.0f};

		glm::mat4 matrix() const;
	};

	Data const& data() const { return m_data; }

	glm::vec2 position() const { return m_data.position; }
	Radian rotation() const { return m_data.rotation; }
	glm::vec2 scale() const { return m_data.scale; }
	glm::mat4 const& matrix() const { return m_dirty ? refresh() : m_matrix; }

	Transform& setPosition(glm::vec2 xy);
	Transform& setRotation(Radian rad);
	Transform& setScale(nvec2 xy);

	Transform& setDirty();
	glm::mat4 const& refresh() const;

  private:
	Data m_data{};
	mutable glm::mat4 m_matrix{};
	mutable bool m_dirty{};
};

// impl

inline glm::mat4 Transform::Data::matrix() const {
	auto ret = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f));
	ret = glm::rotate(ret, rotation.value, glm::vec3(0.0f, 0.0f, 1.0f));
	return glm::scale(ret, glm::vec3(scale, 1.0f));
}

inline Transform& Transform::setPosition(glm::vec2 const xy) {
	m_data.position = xy;
	return setDirty();
}

inline Transform& Transform::setRotation(Radian const rad) {
	m_data.rotation = rad;
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

inline glm::mat4 const& Transform::refresh() const {
	m_dirty = false;
	return m_matrix = m_data.matrix();
}
} // namespace vf
