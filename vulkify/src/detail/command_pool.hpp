#pragma once
#include <detail/defer_queue.hpp>
#include <detail/vulkan_device.hpp>
#include <mutex>

namespace vf {
class FencePool {
  public:
	FencePool(VulkanDevice device = {}, std::size_t count = 0);

	vk::Fence next();

  private:
	vk::UniqueFence make_fence() const;

	std::vector<vk::UniqueFence> m_idle{};
	std::vector<vk::UniqueFence> m_busy{};
	VulkanDevice m_device{};
};

class CommandPool {
  public:
	static constexpr auto pool_flags_v = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;

	CommandPool(VulkanDevice device = {}, std::size_t batch = 4);

	vk::CommandBuffer acquire();
	vk::Result release(vk::CommandBuffer&& cb, bool block, DeferQueue&& defer = {});

	void clear();

	VulkanDevice m_device{};

  private:
	struct Cmd {
		DeferQueue defer{};
		vk::CommandBuffer cb{};
		vk::Fence fence{};
	};

	FencePool m_fencePool;
	std::vector<Cmd> m_cbs{};
	vk::UniqueCommandPool m_pool{};
	std::uint32_t m_batch{};
};

struct CommandPoolFactory {
	VulkanDevice device{};
	CommandPool operator()() const { return device; }
};
} // namespace vf
