#pragma once
#include <glm/vec2.hpp>
#include <vulkify/core/ptr.hpp>
#include <vulkify/instance/instance_enums.hpp>
#include <vector>

namespace vf {
struct Gpu;

enum class InstanceFlag { eAutoShow, eLinearSwapchain, eSuperSampling, eHeadless };
using InstanceFlags = ktl::enum_flags<InstanceFlag>;

struct GpuSelector {
	virtual Gpu const* operator()(Gpu const* first, Gpu const* last) const = 0;
};

struct InstanceCreateInfo {
	char const* title{};
	glm::uvec2 extent{1280, 720};
	WindowFlags windowFlags{};
	InstanceFlags instanceFlags{};
	AntiAliasing desiredAA{AntiAliasing::e2x};
	Ptr<GpuSelector const> gpuSelector{};
	std::vector<VSync> desiredVsyncs{VSync::eAdaptive, VSync::eOn};
};
} // namespace vf
