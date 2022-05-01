#pragma once
#include <glm/vec2.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <string>

namespace vf {
struct InstanceCreateInfo {
	enum class Flag { eBorderless, eNoResize, eHidden, eMaximized, eLinearSwapchain, eHeadless };
	using Flags = ktl::enum_flags<Flag, std::uint8_t>;

	std::string title{"(Untitled)"};
	glm::uvec2 extent{1280, 720};
	Flags flags{};
};
} // namespace vf
