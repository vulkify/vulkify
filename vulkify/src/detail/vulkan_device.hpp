#pragma once
#include <ktl/enum_flags/enum_flags.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkify/instance/instance_enums.hpp>
#include <limits>
#include <mutex>

namespace vf::refactor {
constexpr VSync to_vsync(vk::PresentModeKHR mode) {
	switch (mode) {
	default:
	case vk::PresentModeKHR::eFifo: return VSync::eOn;
	case vk::PresentModeKHR::eFifoRelaxed: return VSync::eAdaptive;
	case vk::PresentModeKHR::eMailbox: return VSync::eTripleBuffer;
	case vk::PresentModeKHR::eImmediate: return VSync::eOff;
	}
}

constexpr vk::PresentModeKHR from_vsync(VSync vsync) {
	switch (vsync) {
	default:
	case VSync::eOn: return vk::PresentModeKHR::eFifo;
	case VSync::eAdaptive: return vk::PresentModeKHR::eFifoRelaxed;
	case VSync::eTripleBuffer: return vk::PresentModeKHR::eMailbox;
	case VSync::eOff: return vk::PresentModeKHR::eImmediate;
	}
}

struct ImageView {
	vk::Image image{};
	vk::ImageView view{};
	vk::Extent2D extent{};
};

struct Queue {
	vk::Queue queue{};
	std::uint32_t family{};
};

struct SwapchainSync {
	vk::Semaphore draw{};
	vk::Semaphore present{};
	vk::Fence drawn{};
};

struct DescriptorSetLayout {
	vk::DescriptorSetLayout layout{};
	vk::BufferUsageFlagBits bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer;
};

struct VulkanInstance;

struct VulkanDevice {
	static constexpr auto fence_wait_v = std::numeric_limits<std::uint64_t>::max();

	enum class Flag { eDebugMsgr, eLinearSwp };
	using Flags = ktl::enum_flags<Flag, std::uint8_t>;

	Queue queue{};
	vk::PhysicalDevice gpu{};
	vk::Device device{};
	std::mutex* queue_mutex{};
	vk::PhysicalDeviceLimits const* limits{};
	Flags flags{};

	static VulkanDevice make(VulkanInstance const& instance);

	explicit operator bool() const { return device; }

	bool busy(vk::Fence fence) const { return fence && device.getFenceStatus(fence) == vk::Result::eNotReady; }
	void wait(vk::Fence fence, std::uint64_t wait = fence_wait_v) const;
	void reset(vk::Fence fence, std::uint64_t wait = fence_wait_v) const;

	vk::UniqueImageView make_image_view(vk::Image const image, vk::Format const format, vk::ImageAspectFlags aspects) const;
};
} // namespace vf::refactor
