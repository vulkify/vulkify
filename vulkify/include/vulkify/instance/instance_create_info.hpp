#pragma once
#include <glm/vec2.hpp>
#include <vulkify/instance/window_flags.hpp>

namespace vf {
enum class InstanceFlag { eAutoShow, eLinearSwapchain, eHeadless };
using InstanceFlags = ktl::enum_flags<InstanceFlag>;

struct InstanceCreateInfo {
	char const* title{};
	glm::uvec2 extent{1280, 720};
	WindowFlags windowFlags{};
	InstanceFlags instanceFlags{};
};
} // namespace vf
