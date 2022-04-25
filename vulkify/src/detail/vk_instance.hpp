#pragma once
#include <detail/command_pool.hpp>
#include <detail/vk_device.hpp>
#include <ktl/async/kfunction.hpp>
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/result.hpp>
#include <mutex>

namespace vf {
using MakeSurface = ktl::kfunction<vk::SurfaceKHR(vk::Instance)>;

struct VKGpu {
	vk::PhysicalDeviceProperties properties{};
	std::vector<vk::SurfaceFormatKHR> formats{};
	vk::PhysicalDevice device{};
};

struct VKSync {
	vk::Semaphore draw{};
	vk::Semaphore present{};
	vk::Fence drawn{};
};

struct VKInstance {
	vk::UniqueInstance instance{};
	vk::UniqueDebugUtilsMessengerEXT messenger{};
	VKGpu gpu{};
	vk::UniqueDevice device{};
	vk::UniqueSurfaceKHR surface{};
	VKQueue queue{};
	ktl::kunique_ptr<DeferQueue> defer{};
	ktl::kunique_ptr<std::mutex> mutex{};
	ktl::kunique_ptr<CommandPool> commandPool{};

	static Result<VKInstance> make(MakeSurface makeSurface, bool validation = true);

	VKDevice makeDevice() { return {queue, gpu.device, *device, {defer.get()}, mutex.get()}; }
};
} // namespace vf
