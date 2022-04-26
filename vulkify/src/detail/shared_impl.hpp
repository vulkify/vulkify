#pragma once
#include <detail/vram.hpp>
#include <glm/mat4x4.hpp>
#include <ktl/unique_val.hpp>
#include <vulkify/context/canvas.hpp>

namespace vf {
struct Rgba;
struct PipelineFactory;
class Instance;

struct ShaderInput {
	vk::DescriptorSet set{};
	std::uint32_t number{};
};

struct Canvas::Impl {
	ktl::unique_val<Instance*> instance{};
	PipelineFactory* pipelineFactory{};
	vk::RenderPass renderPass{};
	vk::CommandBuffer commandBuffer{};
	ShaderInput mat{};
	Rgba* clear{};
};

struct GfxResource {
	Vram vram{};
	BufferObject buffer{};
};
} // namespace vf
