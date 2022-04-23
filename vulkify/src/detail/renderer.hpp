#pragma once
#include <detail/rotator.hpp>
#include <detail/vk_instance.hpp>
#include <detail/vk_surface.hpp>
#include <detail/vram.hpp>
#include <glm/vec2.hpp>

namespace vf {
struct FrameSync {
	vk::UniqueSemaphore draw{};
	vk::UniqueSemaphore present{};
	vk::UniqueFence drawn{};
	vk::UniqueFramebuffer framebuffer{};
	struct {
		vk::UniqueCommandBuffer primary{};
		vk::UniqueCommandBuffer secondary{};
	} cmd{};

	static FrameSync make(vk::Device device);

	VKSync sync() const { return {*draw, *present, *drawn}; }
};

struct RenderTarget {
	VKImage colour{};
	VKImage depth{};
};

struct Renderer {
	Vram vram{};
	vk::UniqueRenderPass renderPass{};
	vk::UniqueCommandPool commandPool{};
	Rotator<FrameSync> frameSync{};
	VKImage target{};

	static Renderer make(Vram vram, VKSurface const& surface, std::size_t buffering = 2);

	VKSync sync() const { return frameSync.get().sync(); }
	vk::CommandBuffer beginPass(VKImage const& target);
	vk::CommandBuffer endPass();
	void next() { frameSync.next(); }

	vk::UniqueFramebuffer makeFramebuffer(RenderTarget const& target) const;
};
} // namespace vf
