#pragma once
#include <ktl/expected.hpp>

namespace vf {
enum class Error {
	eDuplicateInstance,
	eNoVulkanSupport,
	eGlfwFailure,
};

template <typename T>
using Result = ktl::expected<T, Error>;
} // namespace vf
