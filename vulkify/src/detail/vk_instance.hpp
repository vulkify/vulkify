#pragma once
#include <detail/defer.hpp>
#include <ktl/async/kfunction.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkify/core/result.hpp>

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
	VKGpu gpu{};
	VKQueue queue{};
	vk::Device device{};
	Defer* defer{};
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

	VKDevice makeDevice() { return {gpu, queue, *device, &defer}; }
};

vk::UniqueImageView makeImageView(vk::Device device, vk::Image const image, vk::Format const format, vk::ImageAspectFlags aspects);
} // namespace vf