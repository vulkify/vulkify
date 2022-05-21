#pragma once
#include <detail/vk_device.hpp>
#include <vulkify/core/rgba.hpp>

namespace vf {
struct ImageBarrier;

struct RenderTarget {
	VKImage colour{};
	VKImage resolve{};
	vk::Extent2D extent{};
};

struct Framebuffer : RenderTarget {
	vk::Framebuffer framebuffer{};
	operator vk::Framebuffer() const { return framebuffer; }
	explicit operator bool() const { return framebuffer; }
};

struct Renderer {
	vk::UniqueRenderPass renderPass{};
	vk::Device device{};

	static constexpr bool isSrgb(vk::Format const f) {
		using F = vk::Format;
		return f == F::eR8G8Srgb || f == F::eB8G8R8Srgb || f == F::eR8G8B8Srgb || f == F::eB8G8R8A8Srgb || f == F::eR8G8B8A8Srgb;
	}

	static Renderer make(vk::Device device, vk::Format colour, vk::SampleCountFlagBits samples);

	vk::UniqueFramebuffer makeFramebuffer(RenderTarget const& target) const;
	void undefToColour(vk::CommandBuffer primary, std::span<VKImage const> images) const;
	void render(Framebuffer const& framebuffer, Rgba clear, vk::CommandBuffer primary, std::span<vk::CommandBuffer const> recorded) const;
	void blit(vk::CommandBuffer primary, VKImage const& from, VKImage const& to) const;
	void colourToPresent(vk::CommandBuffer primary, VKImage const& image) const;
	void tfrToPresent(vk::CommandBuffer primary, VKImage const& image) const;
};
} // namespace vf
