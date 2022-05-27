#pragma once
#include <detail/vk_device.hpp>
#include <ktl/async/kfunction.hpp>
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/defines.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/instance/gpu.hpp>
#include <mutex>

namespace vf {
using MakeSurface = ktl::kfunction<vk::SurfaceKHR(vk::Instance)>;

struct VKGpu {
	vk::PhysicalDeviceProperties properties{};
	std::vector<vk::SurfaceFormatKHR> formats{};
	vk::PhysicalDevice device{};
	Gpu gpu{};
};

struct PhysicalDevice {
	Gpu gpu{};
	vk::PhysicalDevice device{};
	std::uint32_t queueFamily{};
	int score{};

	auto operator<=>(PhysicalDevice const& rhs) const { return score <=> rhs.score; }
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

	struct Builder;

	vk::UniqueInstance instance{};
	vk::UniqueDebugUtilsMessengerEXT messenger{};
	VKGpu gpu{};
	vk::UniqueDevice device{};
	vk::UniqueSurfaceKHR surface{};
	VKQueue queue{};
	ktl::kunique_ptr<Util> util{};

	std::vector<Gpu> availableDevices() const;
};

struct VKInstance::Builder {
	VKInstance instance{};
	std::vector<PhysicalDevice> devices{};
	bool validation{};

	static Result<Builder> make(Info info, bool validation = debug_v);

	Result<VKInstance> operator()(PhysicalDevice&& selected);
};
} // namespace vf
