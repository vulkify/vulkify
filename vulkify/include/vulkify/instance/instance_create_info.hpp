#pragma once
#include <glm/vec2.hpp>
#include <vulkify/core/ptr.hpp>
#include <vulkify/instance/gpu.hpp>
#include <vulkify/instance/instance_enums.hpp>
#include <span>

namespace vf {
enum class InstanceFlag { eAutoShow, eLinearSwapchain, eSuperSampling, eHeadless };
using InstanceFlags = ktl::enum_flags<InstanceFlag>;

struct GpuSelector {
	virtual std::size_t operator()(std::span<Gpu const> available) const = 0;
};

struct InstanceCreateInfo {
	char const* title{};
	glm::uvec2 extent{1280, 720};
	WindowFlags windowFlags{};
	InstanceFlags instanceFlags{};
	AntiAliasing desiredAA{AntiAliasing::e2x};
	Ptr<GpuSelector const> gpuSelector{};
};
} // namespace vf
