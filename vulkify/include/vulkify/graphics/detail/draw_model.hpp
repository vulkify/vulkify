#pragma once
#include <glm/vec4.hpp>

namespace vf {
///
/// \brief Object SSBO
///
/// Note: this data is uploaded directly to storage buffers
///
struct DrawModel {
	glm::vec4 pos_orn;
	glm::vec4 scl_z_tint;
};
} // namespace vf
