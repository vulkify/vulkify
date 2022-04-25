#pragma once
#include <detail/vram.hpp>
#include <ktl/either.hpp>
#include <ktl/unique_val.hpp>
#include <vulkan/vulkan.hpp>
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
	ktl::either<UniqueBuffer, UniqueImage> resource{};
};
} // namespace vf
