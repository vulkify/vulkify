#pragma once
#include <detail/vk_instance.hpp>
#include <detail/vram.hpp>

namespace vf {
struct Renderer {
	Vram vram{};
	vk::UniqueRenderPass renderPass{};

	static Renderer make(VKGpu const& gpu, Vram vram, vk::SurfaceKHR surface, bool autoTransition);
};
} // namespace vf
