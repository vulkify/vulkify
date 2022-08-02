#pragma once
#include <detail/defer_queue.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkify/core/time.hpp>
#include <mutex>

namespace vf {
struct VKImage {
	vk::Image image{};
	vk::ImageView view{};
	vk::Extent2D extent{};
};

struct VKQueue {
	vk::Queue queue{};
	std::uint32_t family{};
};

struct DescriptorLayout {
	vk::DescriptorSetLayout layout{};
	vk::BufferUsageFlagBits bufferUsage = vk::BufferUsageFlagBits::eUniformBuffer;
};

struct VKDevice {
	static constexpr auto fence_wait_v = std::numeric_limits<std::uint64_t>::max();

	enum class Flag { eDebugMsgr, eLinearSwp };
	using Flags = ktl::enum_flags<Flag, std::uint8_t>;

	VKQueue queue{};
	vk::PhysicalDevice gpu{};
	vk::Device device{};
	Defer defer{};
	std::mutex* queue_mutex{};
	Flags flags{};

	explicit operator bool() const { return device; }

	bool busy(vk::Fence fence) const { return fence && device.getFenceStatus(fence) == vk::Result::eNotReady; }

	void wait(vk::Fence fence, std::uint64_t wait = fence_wait_v) const;
	void reset(vk::Fence fence, std::uint64_t wait = fence_wait_v) const;

	vk::UniqueImageView make_image_view(vk::Image const image, vk::Format const format, vk::ImageAspectFlags aspects) const {
		vk::ImageViewCreateInfo info;
		info.viewType = vk::ImageViewType::e2D;
		info.format = format;
		info.components.r = info.components.g = info.components.b = info.components.a = vk::ComponentSwizzle::eIdentity;
		info.subresourceRange = {aspects, 0, 1, 0, 1};
		info.image = image;
		return device.createImageViewUnique(info);
	}

	template <typename T>
	void set_debug_name(vk::ObjectType type, T const handle, char const* name) const {
		if (flags.test(Flag::eDebugMsgr)) { device.setDebugUtilsObjectNameEXT({type, reinterpret_cast<std::uint64_t>(handle), name}); }
	}
};
} // namespace vf
