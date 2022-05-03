#pragma once
#include <detail/vk_instance.hpp>
#include <glm/vec2.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/core/result.hpp>
#include <optional>

namespace vf {
struct VKSwapchain {
	ktl::fixed_vector<VKImage, 8> images{};
	ktl::fixed_vector<vk::UniqueImageView, 8> views{};
	vk::UniqueSwapchainKHR swapchain{};

	explicit operator bool() const { return swapchain.get(); }
	bool operator==(VKSwapchain const& rhs) const { return !swapchain && !rhs.swapchain; }
};

enum class PresentOutcome { eSuccess, eNotReady };
using PresentResult = ktl::expected<PresentOutcome, vk::Result>;

struct VKSurface {
	struct Acquire {
		VKImage image{};
		std::optional<std::uint32_t> index{};

		explicit operator bool() const { return index.has_value(); }
	};

	using Device = VKDevice;

	static constexpr vk::Result refresh_v[] = {vk::Result::eErrorOutOfDateKHR, vk::Result::eSuboptimalKHR};

	VKDevice device{};
	VKGpu gpu{};
	vk::SurfaceKHR surface{};
	bool linear{};

	vk::SwapchainCreateInfoKHR info{};
	VKSwapchain swapchain{};

	static VKSurface make(VKDevice const& device, VKGpu const& gpu, vk::SurfaceKHR surface, glm::ivec2 framebuffer, bool linear);

	explicit operator bool() const { return device.device && surface; }

	vk::SwapchainCreateInfoKHR makeInfo(glm::ivec2 framebuffer) const;
	vk::Result refresh(glm::ivec2 framebuffer);
	Acquire acquire(vk::Semaphore signal, glm::ivec2 framebuffer);
	vk::Result submit(vk::CommandBuffer cb, VKSync const& sync);
	PresentResult present(Acquire const& acquired, vk::Semaphore wait, glm::ivec2 framebuffer);
};
} // namespace vf
