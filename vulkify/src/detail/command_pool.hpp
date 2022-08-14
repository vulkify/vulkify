#pragma once
#include <detail/vulkan_device.hpp>
#include <ktl/kunique_ptr.hpp>
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
	vk::Result release(vk::CommandBuffer&& cb, bool block);

	void clear();

	VulkanDevice m_device{};

  private:
	struct Cmd {
		vk::CommandBuffer cb{};
		vk::Fence fence{};
	};

	FencePool m_fence_pool;
	std::vector<Cmd> m_cbs{};
	vk::UniqueCommandPool m_pool{};
	ktl::kunique_ptr<std::mutex> m_mutex{};
	std::uint32_t m_batch{};
};

struct CommandPoolFactory {
	VulkanDevice device{};
	CommandPool operator()() const { return device; }
};
} // namespace vf
