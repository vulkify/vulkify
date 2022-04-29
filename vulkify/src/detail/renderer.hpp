#pragma once
#include <detail/image_cache.hpp>
#include <detail/rotator.hpp>
#include <detail/vk_device.hpp>
#include <detail/vk_surface.hpp>
#include <detail/vram.hpp>
#include <glm/vec2.hpp>
#include <vulkify/core/rgba.hpp>

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
	ImageCache depthImage{};
	RenderTarget attachments{};
	Rgba clear{};
	vk::Format colour{};

	static constexpr bool isSrgb(vk::Format const f) {
		using F = vk::Format;
		return f == F::eR8G8Srgb || f == F::eB8G8R8Srgb || f == F::eR8G8B8Srgb || f == F::eB8G8R8A8Srgb || f == F::eR8G8B8A8Srgb;
	}

	static Renderer make(Vram vram, VKSurface const& surface, std::size_t buffering = 2);

	bool isSrgb() const { return isSrgb(colour); }
	vk::Format textureFormat() const { return isSrgb() ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm; }

	VKSync sync() const { return frameSync.get().sync(); }
	vk::CommandBuffer beginPass(VKImage const& target);
	vk::CommandBuffer endPass();
	void next() { frameSync.next(); }

	vk::UniqueFramebuffer makeFramebuffer(RenderTarget const& target) const;
	vk::CommandBuffer drawCmd() const { return *frameSync.get().cmd.secondary; }
};
} // namespace vf
