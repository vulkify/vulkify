#pragma once
#include <detail/defer_queue.hpp>
#include <detail/gfx_device.hpp>
#include <mutex>

namespace vf::refactor {
class FencePool {
  public:
	FencePool(GfxDevice device = {}, std::size_t count = 0) : m_device(device) {
		if (!m_device) { return; }
		m_idle.reserve(count);
		for (std::size_t i = 0; i < count; ++i) { m_idle.push_back(make_fence()); }
	}

	vk::Fence next() {
		if (!m_device) { return {}; }
		std::erase_if(m_busy, [this](vk::UniqueFence& f) {
			if (!m_device.device.busy(*f)) {
				m_idle.push_back(std::move(f));
				return true;
			}
			return false;
		});
		if (m_idle.empty()) { m_idle.push_back(make_fence()); }
		auto ret = std::move(m_idle.back());
		m_idle.pop_back();
		m_device.device.reset(*ret, {});
		m_busy.push_back(std::move(ret));
		return *m_busy.back();
	}

  private:
	vk::UniqueFence make_fence() const { return m_device.device.device.createFenceUnique({vk::FenceCreateFlagBits::eSignaled}); }

	std::vector<vk::UniqueFence> m_idle{};
	std::vector<vk::UniqueFence> m_busy{};
	GfxDevice m_device{};
};

class CommandPool {
  public:
	static constexpr auto pool_flags_v = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;

	CommandPool(GfxDevice device = {}, std::size_t batch = 4) : m_device(device), m_fencePool(device, 0U), m_batch(static_cast<std::uint32_t>(batch)) {
		if (!m_device) { return; }
		m_pool = m_device.device.device.createCommandPoolUnique({pool_flags_v, device.device.queue.family});
		assert(m_pool);
	}

	vk::CommandBuffer acquire() {
		if (!m_device || !m_pool) { return {}; }
		Cmd cmd;
		for (auto& cb : m_cbs) {
			if (!m_device.device.busy(cb.fence)) {
				std::swap(cb, m_cbs.back());
				cmd = std::move(m_cbs.back());
				break;
			}
		}
		if (!cmd.cb) {
			auto const info = vk::CommandBufferAllocateInfo(*m_pool, vk::CommandBufferLevel::ePrimary, m_batch);
			auto cbs = m_device.device.device.allocateCommandBuffers(info);
			m_cbs.reserve(m_cbs.size() + cbs.size());
			for (auto const cb : cbs) { m_cbs.push_back(Cmd{{}, cb, {}}); }
			cmd = std::move(m_cbs.back());
		}
		m_cbs.pop_back();
		auto ret = cmd.cb;
		ret.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		return ret;
	}

	vk::Result release(vk::CommandBuffer&& cb, bool block, DeferQueue&& defer = {}) {
		auto ret = vk::Result::eErrorDeviceLost;
		if (!m_device) { return ret; }
		assert(!std::any_of(m_cbs.begin(), m_cbs.end(), [c = cb](Cmd const& cmd) { return cmd.cb == c; }));
		cb.end();
		Cmd cmd{std::move(defer), cb, m_fencePool.next()};
		vk::SubmitInfo const si(0U, nullptr, {}, 1U, &cb);
		{
			auto lock = std::scoped_lock(*m_device.device.queue_mutex);
			ret = m_device.device.queue.queue.submit(1, &si, cmd.fence);
		}
		if (ret == vk::Result::eSuccess) {
			if (block) {
				m_device.device.wait(cmd.fence);
				cmd.defer.entries.clear();
			}
		} else {
			m_device.device.reset(cmd.fence, {});
		}
		m_cbs.push_back(std::move(cmd));
		return ret;
	}

	void clear() {
		m_device.device.device.waitIdle();
		m_cbs.clear();
	}

	GfxDevice m_device{};

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
} // namespace vf::refactor
