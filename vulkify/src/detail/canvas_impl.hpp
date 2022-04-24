#pragma once
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
} // namespace vf
