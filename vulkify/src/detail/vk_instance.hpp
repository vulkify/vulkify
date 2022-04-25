#pragma once
#include <detail/defer.hpp>
#include <ktl/async/kfunction.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/core/time.hpp>

namespace vf {
using MakeSurface = ktl::kfunction<vk::SurfaceKHR(vk::Instance)>;

struct VKGpu {
	vk::PhysicalDeviceProperties properties{};
	std::vector<vk::SurfaceFormatKHR> formats{};
	vk::PhysicalDevice device{};
};

struct VKQueue {
	vk::Queue queue{};
	std::uint32_t family{};
};

struct VKDevice {
	static constexpr auto fence_wait_v = 2s;

	VKQueue queue{};
	vk::PhysicalDevice gpu{};
	vk::Device device{};
	Defer* defer{};

	bool busy(vk::Fence fence) const { return fence && device.getFenceStatus(fence) == vk::Result::eNotReady; }

	void wait(vk::Fence fence, stdch::nanoseconds wait = fence_wait_v) const {
		if (fence) { device.waitForFences(fence, true, static_cast<std::uint64_t>(wait.count())); }
	}

	void reset(vk::Fence fence, bool wait = true) const {
		if (wait && busy(fence)) { this->wait(fence); }
		device.resetFences(fence);
	}
};

struct VKSync {
	vk::Semaphore draw{};
	vk::Semaphore present{};
	vk::Fence drawn{};
};

struct VKImage {
	vk::Image image{};
	vk::ImageView view{};
	vk::Extent2D extent{};
};

struct VKInstance {
	vk::UniqueInstance instance{};
	vk::UniqueDebugUtilsMessengerEXT messenger{};
	VKGpu gpu{};
	vk::UniqueDevice device{};
	vk::UniqueSurfaceKHR surface{};
	VKQueue queue{};
	Defer defer{};

	static Result<VKInstance> make(MakeSurface makeSurface, bool validation = true);

	VKDevice makeDevice() { return {queue, gpu.device, *device, &defer}; }
};

vk::UniqueImageView makeImageView(vk::Device device, vk::Image const image, vk::Format const format, vk::ImageAspectFlags aspects);
} // namespace vf
