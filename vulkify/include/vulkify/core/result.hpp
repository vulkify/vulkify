#pragma once
#include <ktl/expected.hpp>

namespace vf {
///
/// \brief Set of all Vulkify error types
///
enum class Error {
	eUnknown,

	eInvalidArgument,
	eDuplicateInstance,
	eNoVulkanSupport,
	eGlfwFailure,
	eVulkanInitFailure,
	eFreetypeInitFailure,

	eInactiveInstance,
	eMemoryError,
	eIOError,
};

///
/// \brief T or an Error
///
template <typename T>
using Result = ktl::expected<T, Error>;
} // namespace vf
