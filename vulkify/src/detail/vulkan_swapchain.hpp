#pragma once
#include <detail/vulkan_device.hpp>
#include <glm/vec2.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/core/result.hpp>
#include <optional>

namespace vf::refactor {
struct GfxDevice;

struct Swapchain {
	ktl::fixed_vector<ImageView, 8> images{};
	ktl::fixed_vector<vk::UniqueImageView, 8> views{};
	vk::UniqueSwapchainKHR swapchain{};

	explicit operator bool() const { return swapchain.get(); }
	bool operator==(Swapchain const& rhs) const { return !swapchain && !rhs.swapchain; }
};

struct VulkanSwapchain {
	struct Acquire {
		ImageView image{};
		std::optional<std::uint32_t> index{};

		explicit operator bool() const { return index.has_value(); }
	};

	static constexpr vk::Result refresh_v[] = {vk::Result::eErrorOutOfDateKHR, vk::Result::eSuboptimalKHR};

	GfxDevice const* device{};
	vk::SurfaceKHR surface{};
	std::span<vk::SurfaceFormatKHR const> formats{};
	bool linear{};

	vk::SwapchainCreateInfoKHR info{};
	Swapchain swapchain{};

	static VulkanSwapchain make(GfxDevice const* device, std::span<vk::SurfaceFormatKHR const> formats, vk::SurfaceKHR surface, vk::PresentModeKHR mode,
								glm::ivec2 extent, bool linear);

	explicit operator bool() const { return device && surface; }

	vk::SwapchainCreateInfoKHR make_info(glm::uvec2 extent) const;
	vk::Result refresh(glm::uvec2 extent, vk::PresentModeKHR mode);
	Acquire acquire(vk::Semaphore signal, glm::uvec2 extent);
	void submit(vk::CommandBuffer cb, SwapchainSync const& sync);
	void present(Acquire const& acquired, vk::Semaphore wait, glm::uvec2 extent);
};
} // namespace vf::refactor
