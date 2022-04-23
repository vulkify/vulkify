#pragma once
#include <ktl/async/kfunction.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkify/core/result.hpp>

namespace vf {
using MakeSurface = ktl::kfunction<vk::SurfaceKHR(vk::Instance)>;

struct VKGpu {
	vk::PhysicalDeviceProperties properties;
	std::vector<vk::SurfaceFormatKHR> formats;
	vk::PhysicalDevice device;
};

struct VKQueue {
	vk::Queue queue;
	std::uint32_t family{};
};

struct VKInstance {
	vk::UniqueInstance instance;
	vk::UniqueDebugUtilsMessengerEXT messenger;
	VKGpu gpu;
	vk::UniqueDevice device;
	vk::UniqueSurfaceKHR surface;
	VKQueue queue;

	static Result<VKInstance> make(MakeSurface makeSurface, bool validation);
};
} // namespace vf
