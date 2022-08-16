#pragma once
#include <detail/vulkan_device.hpp>
#include <vulkify/core/rgba.hpp>

namespace vf {
struct ImageBarrier;

struct RenderTarget {
	ImageView colour{};
	ImageView depth{};
	ImageView resolve{};
	vk::Extent2D extent{};
};

struct Framebuffer : RenderTarget {
	vk::Framebuffer framebuffer{};
	operator vk::Framebuffer() const { return framebuffer; }
	explicit operator bool() const { return framebuffer; }
};

struct Renderer {
	struct Frame;

	vk::UniqueRenderPass render_pass{};
	vk::Device device{};

	static constexpr bool isSrgb(vk::Format const f) {
		using F = vk::Format;
		return f == F::eR8G8Srgb || f == F::eB8G8R8Srgb || f == F::eR8G8B8Srgb || f == F::eB8G8R8A8Srgb || f == F::eR8G8B8A8Srgb;
	}

	static Renderer make(vk::Device device, vk::Format colour, vk::SampleCountFlagBits samples);

	vk::UniqueFramebuffer make_framebuffer(RenderTarget const& target) const;
};

struct Renderer::Frame {
	Renderer& renderer;
	Framebuffer const& framebuffer;
	vk::CommandBuffer cmd;

	void render(Rgba clear, std::span<vk::CommandBuffer const> recorded) const;
	void blit(ImageView const& src, ImageView const& dst) const;

	void undef_to_depth(ImageView const& src) const;
	void undef_to_colour(std::span<ImageView const> images) const;
	void colour_to_tfr(ImageView const& src, ImageView const& dst) const;
	void colour_to_present(ImageView const& image) const;
	void tfr_to_present(ImageView const& image) const;
};
} // namespace vf
