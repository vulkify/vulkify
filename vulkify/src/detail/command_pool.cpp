#include <detail/command_pool.hpp>

namespace vf {
static constexpr vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

FencePool::FencePool(VKDevice device, std::size_t count) : m_device(device) {
	if (!m_device) { return; }
	m_idle.reserve(count);
	for (std::size_t i = 0; i < count; ++i) { m_idle.push_back(makeFence()); }
}

vk::Fence FencePool::next() {
	if (!m_device) { return {}; }
	std::erase_if(m_busy, [this](vk::UniqueFence& f) {
		if (!m_device.busy(*f)) {
			m_idle.push_back(std::move(f));
			return true;
		}
		return false;
	});
	if (m_idle.empty()) { m_idle.push_back(makeFence()); }
	auto ret = std::move(m_idle.back());
	m_idle.pop_back();
	m_device.reset(*ret, false);
	m_busy.push_back(std::move(ret));
	return *m_busy.back();
}

vk::UniqueFence FencePool::makeFence() const { return m_device.device.createFenceUnique({vk::FenceCreateFlagBits::eSignaled}); }

CommandPool::CommandPool(VKDevice device, std::size_t batch) : m_fencePool(device, 0U), m_device(device), m_batch(static_cast<std::uint32_t>(batch)) {
	if (!m_device) { return; }
	m_pool = device.device.createCommandPoolUnique({pool_flags_v, device.queue.family});
	assert(m_pool);
}

vk::CommandBuffer CommandPool::acquire() {
	if (!m_device || !m_pool) { return {}; }
	Cmd cmd;
	for (auto& cb : m_cbs) {
		if (!m_device.busy(cb.fence)) {
			std::swap(cb, m_cbs.back());
			cmd = std::move(m_cbs.back());
			break;
		}
	}
	if (!cmd.cb) {
		auto const info = vk::CommandBufferAllocateInfo(*m_pool, vk::CommandBufferLevel::ePrimary, m_batch);
		auto cbs = m_device.device.allocateCommandBuffers(info);
		m_cbs.reserve(m_cbs.size() + cbs.size());
		for (auto const cb : cbs) { m_cbs.push_back(Cmd{{}, cb, {}}); }
		cmd = std::move(m_cbs.back());
	}
	m_cbs.pop_back();
	auto ret = cmd.cb;
	ret.begin(flags);
	return ret;
}

vk::Result CommandPool::release(vk::CommandBuffer&& cb, bool block, DeferQueue&& defer) {
	auto ret = vk::Result::eErrorDeviceLost;
	if (!m_device) { return ret; }
	assert(!std::any_of(m_cbs.begin(), m_cbs.end(), [c = cb](Cmd const& cmd) { return cmd.cb == c; }));
	cb.end();
	Cmd cmd{std::move(defer), cb, m_fencePool.next()};
	vk::SubmitInfo const si(0U, nullptr, {}, 1U, &cb);
	{
		auto lock = std::scoped_lock(*m_device.mutex);
		ret = m_device.queue.queue.submit(1, &si, cmd.fence);
	}
	if (ret == vk::Result::eSuccess) {
		if (block) {
			m_device.wait(cmd.fence);
			cmd.defer.entries.clear();
		}
	} else {
		m_device.reset(cmd.fence, false);
	}
	m_cbs.push_back(std::move(cmd));
	return ret;
}
} // namespace vf
