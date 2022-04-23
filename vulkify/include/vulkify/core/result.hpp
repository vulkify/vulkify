#pragma once
#include <ktl/expected.hpp>

namespace vf {
enum class Error {
	eInvalidArgument,
	eDuplicateInstance,
	eNoVulkanSupport,
	eGlfwFailure,
	eVulkanInitFailure,
};

template <typename T>
using Result = ktl::expected<T, Error>;
} // namespace vf
