#pragma once
#include <detail/vk_device.hpp>
#include <vulkify/core/rgba.hpp>

namespace vf {
struct RenderImage {
	VKImage colour{};
	VKImage resolve;
};

struct RenderTarget : RenderImage {
	vk::Framebuffer framebuffer{};
};

struct Renderer {
	vk::Device device{};
	vk::UniqueRenderPass renderPass{};

	static constexpr bool isSrgb(vk::Format const f) {
		using F = vk::Format;
		return f == F::eR8G8Srgb || f == F::eB8G8R8Srgb || f == F::eR8G8B8Srgb || f == F::eB8G8R8A8Srgb || f == F::eR8G8B8A8Srgb;
	}

	static Renderer make(vk::Device device, vk::Format colour, vk::SampleCountFlagBits samples);

	vk::UniqueFramebuffer makeFramebuffer(RenderImage const& image) const;
	void toColourOptimal(vk::CommandBuffer primary, std::span<vk::Image const> images) const;
	void render(RenderTarget const& target, Rgba clear, vk::CommandBuffer primary, std::span<vk::CommandBuffer const> recorded) const;
	void toPresentSrc(vk::CommandBuffer primary, vk::Image present) const;
};
} // namespace vf
