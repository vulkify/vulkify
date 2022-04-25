#pragma once
#include <detail/defer_queue.hpp>
#include <detail/vk_device.hpp>

namespace vf {
class FencePool {
  public:
	FencePool(VKDevice device = {}, std::size_t count = 0);

	vk::Fence next();

  private:
	vk::UniqueFence makeFence() const;

	std::vector<vk::UniqueFence> m_idle{};
	std::vector<vk::UniqueFence> m_busy{};
	VKDevice m_device{};
};

class CommandPool {
  public:
	static constexpr auto pool_flags_v = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;

	CommandPool(VKDevice device = {}, std::size_t batch = 4);

	vk::CommandBuffer acquire();
	vk::Result release(vk::CommandBuffer&& cmd, bool block, DeferQueue&& defer = {});

	void clear() { m_cbs.clear(); }

  private:
	struct Cmd {
		DeferQueue defer{};
		vk::CommandBuffer cb{};
		vk::Fence fence{};
	};

	FencePool m_fencePool;
	std::vector<Cmd> m_cbs{};
	vk::UniqueCommandPool m_pool{};
	VKDevice m_device{};
	std::uint32_t m_batch{};
};
} // namespace vf
