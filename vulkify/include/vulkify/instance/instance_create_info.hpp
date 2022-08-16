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
	WindowFlags window_flags{};
	InstanceFlags instance_flags{};
	AntiAliasing desired_aa{AntiAliasing::e2x};
	Ptr<GpuSelector const> gpu_selector{};
	std::vector<VSync> desired_vsyncs{VSync::eAdaptive, VSync::eOn};
	ZOrder default_z_order{ZOrder::eOff};
};
} // namespace vf
