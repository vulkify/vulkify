#pragma once
#include <detail/defer_queue.hpp>
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

struct VKDevice {
	static constexpr auto fence_wait_v = 2s;

	VKQueue queue{};
	vk::PhysicalDevice gpu{};
	vk::Device device{};
	Defer defer{};
	std::mutex* mutex{};

	explicit operator bool() const { return device; }

	bool busy(vk::Fence fence) const { return fence && device.getFenceStatus(fence) == vk::Result::eNotReady; }

	void wait(vk::Fence fence, stdch::nanoseconds wait = fence_wait_v) const {
		if (fence) { device.waitForFences(fence, true, static_cast<std::uint64_t>(wait.count())); }
	}

	void reset(vk::Fence fence, bool wait = true) const {
		if (wait && busy(fence)) { this->wait(fence); }
		device.resetFences(fence);
	}

	vk::UniqueImageView makeImageView(vk::Image const image, vk::Format const format, vk::ImageAspectFlags aspects) const {
		vk::ImageViewCreateInfo info;
		info.viewType = vk::ImageViewType::e2D;
		info.format = format;
		// TODO: research
		// info.components.r = vk::ComponentSwizzle::eR;
		// info.components.g = vk::ComponentSwizzle::eG;
		// info.components.b = vk::ComponentSwizzle::eB;
		// info.components.a = vk::ComponentSwizzle::eA;
		info.components.r = info.components.g = info.components.b = info.components.a = vk::ComponentSwizzle::eIdentity;
		info.subresourceRange = {aspects, 0, 1, 0, 1};
		info.image = image;
		return device.createImageViewUnique(info);
	}
};
} // namespace vf
