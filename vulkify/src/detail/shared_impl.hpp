#pragma once
#include <detail/vram.hpp>
#include <ktl/unique_val.hpp>
#include <vulkify/context/canvas.hpp>

namespace vf {
struct Rgba;
struct PipelineFactory;
class Instance;

struct Canvas::Impl {
	ktl::unique_val<Instance*> instance{};
	PipelineFactory* pipelineFactory{};
	vk::RenderPass renderPass{};
	vk::CommandBuffer commandBuffer{};
	Rgba* clear{};
};

struct GfxResource {
	Vram vram{};
	BufferObject buffer{};
};
} // namespace vf
