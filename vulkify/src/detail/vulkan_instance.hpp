#pragma once
#include <detail/vulkan_device.hpp>
#include <ktl/async/kfunction.hpp>
#include <vulkify/core/defines.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/instance/gpu.hpp>
#include <mutex>
#include <span>

namespace vf::refactor {
using MakeSurface = ktl::kfunction<vk::SurfaceKHR(vk::Instance)>;

struct GpuInfo {
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

// Move to surface
struct VKSync {
	vk::Semaphore draw{};
	vk::Semaphore present{};
	vk::Fence drawn{};
};

struct VulkanInstance {
	struct Util {
		vk::PhysicalDeviceLimits device_limits{};
		DeferQueue defer{};
		struct {
			std::mutex queue{};
			std::mutex render{};
		} mutex{};
	};

	struct Info {
		std::vector<char const*> instance_extensions{};
		MakeSurface make_surface{};
		std::span<VSync const> desired_vsyncs{};
	};

	struct Builder;

	vk::UniqueInstance instance{};
	vk::UniqueDebugUtilsMessengerEXT messenger{};
	GpuInfo gpu{};
	vk::UniqueDevice device{};
	vk::UniqueSurfaceKHR surface{};
	Queue queue{};
	ktl::kunique_ptr<Util> util{};

	std::vector<Gpu> available_devices() const;
};

struct VulkanInstance::Builder {
	VulkanInstance instance{};
	std::vector<PhysicalDevice> devices{};
	bool validation{};

	static Result<Builder> make(Info info, bool validation = debug_v);

	Result<VulkanInstance> operator()(PhysicalDevice&& selected);
};
} // namespace vf::refactor
