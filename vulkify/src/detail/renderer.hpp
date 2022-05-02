#pragma once
#include <detail/vk_device.hpp>
#include <vulkify/core/rgba.hpp>

namespace vf {
struct RenderImage {
	VKImage colour{};
	VKImage depth{};
};

struct RenderTarget : RenderImage {
	vk::Framebuffer framebuffer{};
};

struct Renderer {
	enum Image { eColour, eDepth };

	vk::Device device{};
	vk::UniqueRenderPass renderPass{};

	static constexpr bool isSrgb(vk::Format const f) {
		using F = vk::Format;
		return f == F::eR8G8Srgb || f == F::eB8G8R8Srgb || f == F::eR8G8B8Srgb || f == F::eB8G8R8A8Srgb || f == F::eR8G8B8A8Srgb;
	}

	static Renderer make(vk::Device device, vk::Format colour, vk::Format depth);

	vk::UniqueFramebuffer makeFramebuffer(RenderImage const& image) const;
	void render(RenderTarget const& target, Rgba clear, vk::CommandBuffer primary, std::span<vk::CommandBuffer const> recorded) const;
	void blit(vk::CommandBuffer primary, RenderTarget const& target, vk::Image dst, vk::ImageLayout final) const;
};
} // namespace vf
