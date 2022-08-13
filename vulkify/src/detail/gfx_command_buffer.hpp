#pragma once
#include <detail/command_pool.hpp>
#include <detail/gfx_device.hpp>
#include <vulkify/core/rect.hpp>

namespace vf {
struct ImageWriter {
	using URegion = TRect<std::uint32_t>;
	using IRegion = TRect<std::int32_t>;

	GfxDevice const* device{};
	vk::CommandBuffer cb;

	ImageWriter(GfxDevice const* device, vk::CommandBuffer cb) : device(device), cb(cb) {}

	std::vector<UniqueBuffer> scratch{};

	static void blit(vk::CommandBuffer cmd, vk::Image in, vk::Image out, TRect<std::int32_t> inr, TRect<std::int32_t> outr, vk::Filter filter);
	static bool can_blit(VmaImage const& src, VmaImage const& dst);

	bool write(VmaImage& out, std::span<std::byte const> data, URegion region = {}, vk::ImageLayout il = {});
	bool blit(VmaImage& in, VmaImage& out, IRegion inr, IRegion outr, vk::Filter filter, TPair<vk::ImageLayout> il = {}) const;
	bool copy(VmaImage& in, VmaImage& out, IRegion inr, IRegion outr, TPair<vk::ImageLayout> il = {}) const;
	void clear(VmaImage& in, Rgba rgba) const;
};

struct GfxCommandBuffer {
	CommandFactory::Scoped pool;
	vk::CommandBuffer cmd;
	ImageWriter writer;

	GfxCommandBuffer(GfxDevice const* device) : pool(*device->command_factory), cmd(pool.get().acquire()), writer(device, cmd) {}
	~GfxCommandBuffer() { pool.get().release(std::move(cmd), true); }

	GfxCommandBuffer& operator=(GfxCommandBuffer&&) = delete;
};
} // namespace vf
