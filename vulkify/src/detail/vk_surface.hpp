#pragma once
#include <detail/vk_instance.hpp>
#include <glm/vec2.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/core/result.hpp>
#include <optional>

namespace vf {
struct VKImage {
	vk::Image image;
	vk::ImageView view;
	vk::Extent2D extent{};
};

struct VKSwapchain {
	ktl::fixed_vector<VKImage, 8> images;
	ktl::fixed_vector<vk::UniqueImageView, 8> views;
	vk::UniqueSwapchainKHR swapchain;
};

enum class PresentOutcome { eSuccess, eNotReady };
using PresentResult = ktl::expected<PresentOutcome, vk::Result>;

struct VKSurface {
	struct Device {
		VKGpu gpu{};
		VKQueue queue{};
		vk::Device device{};
	};

	struct Sync {
		vk::Semaphore wait;
		vk::Semaphore ssignal;
		vk::Fence fsignal;
	};

	struct Acquire {
		VKImage image;
		std::uint32_t index{};
	};

	static constexpr vk::Result refresh_v[] = {vk::Result::eErrorOutOfDateKHR, vk::Result::eSuboptimalKHR};

	vk::SwapchainCreateInfoKHR info;
	VKSwapchain swapchain;
	vk::SurfaceKHR surface;
	class DeferQueue* deferQueue{};

	static vk::SwapchainCreateInfoKHR makeInfo(Device const& device, vk::SurfaceKHR surface, glm::ivec2 framebuffer);

	vk::Result refresh(Device const& device, glm::ivec2 framebuffer);
	std::optional<Acquire> acquire(Device const& device, vk::Semaphore signal, glm::ivec2 framebuffer);
	vk::Result submit(Device const& device, vk::CommandBuffer cb, Sync const& sync);
	PresentResult present(Device const& device, Acquire const& acquired, vk::Semaphore wait, glm::ivec2 framebuffer);
};
} // namespace vf
