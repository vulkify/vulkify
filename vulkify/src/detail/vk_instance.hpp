#pragma once
#include <detail/vk_device.hpp>
#include <ktl/async/kfunction.hpp>
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/defines.hpp>
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
	struct Util {
		DeferQueue defer{};
		std::mutex mutex{};
	};

	struct Info {
		std::vector<char const*> instanceExtensions{};
		MakeSurface makeSurface{};
	};

	vk::UniqueInstance instance{};
	vk::UniqueDebugUtilsMessengerEXT messenger{};
	VKGpu gpu{};
	vk::UniqueDevice device{};
	vk::UniqueSurfaceKHR surface{};
	VKQueue queue{};
	ktl::kunique_ptr<Util> util{};

	static Result<VKInstance> make(Info info, bool validation = debug_v);
};
} // namespace vf
