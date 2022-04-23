#pragma once
#include <detail/vk_instance.hpp>
#include <glm/vec2.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/core/result.hpp>
#include <optional>

namespace vf {
struct VKImage {
	vk::Image image{};
	vk::ImageView view{};
	vk::Extent2D extent{};
};

struct VKSwapchain {
	ktl::fixed_vector<VKImage, 8> images{};
	ktl::fixed_vector<vk::UniqueImageView, 8> views{};
	vk::UniqueSwapchainKHR swapchain{};
};

enum class PresentOutcome { eSuccess, eNotReady };
using PresentResult = ktl::expected<PresentOutcome, vk::Result>;

struct VKSurface {
	struct Acquire {
		VKImage image{};
		std::uint32_t index{};
	};

	using Device = VKDevice;

	static constexpr vk::Result refresh_v[] = {vk::Result::eErrorOutOfDateKHR, vk::Result::eSuboptimalKHR};

	VKDevice device{};
	vk::SwapchainCreateInfoKHR info{};
	VKSwapchain swapchain{};
	vk::SurfaceKHR surface{};
	Defer* defer{};

	static vk::SwapchainCreateInfoKHR makeInfo(VKDevice const& device, vk::SurfaceKHR surface, glm::ivec2 framebuffer);

	vk::Result refresh(glm::ivec2 framebuffer);
	std::optional<Acquire> acquire(vk::Semaphore signal, glm::ivec2 framebuffer);
	vk::Result submit(vk::CommandBuffer cb, VKSync const& sync);
	PresentResult present(Acquire const& acquired, vk::Semaphore wait, glm::ivec2 framebuffer);
};
} // namespace vf
