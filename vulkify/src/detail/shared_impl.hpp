#pragma once
#include <detail/descriptor_set.hpp>
#include <detail/vram.hpp>
#include <ktl/unique_val.hpp>
#include <vulkify/graphics/draw_model.hpp>

namespace vf {
struct PipelineFactory;
class Instance;
struct DrawModel;

struct SetBind {
	std::uint32_t set{};
	std::uint32_t binding{};
};

struct ShaderInput {
	DescriptorSet setZero{};
	SetBind model{};
};

struct RenderPass {
	ktl::unique_val<Instance*> instance{};
	PipelineFactory* pipelineFactory{};
	DescriptorPool* descriptorPool{};
	vk::RenderPass renderPass{};
	vk::CommandBuffer commandBuffer{};
	ShaderInput shaderInput{};
	Rgba* clear{};

	mutable vk::PipelineLayout bound{};

	void writeInstanceData(std::span<DrawModel const> instances, char const* name) const;
	// void writeInstanceData(std::span<DrawInstanceData const> instances, char const* name) const;
};

struct GfxResource {
	Vram vram{};
	BufferObject buffer{};
};

namespace ubo {
struct View {
	glm::mat4 mat_v = glm::mat4(1.0f);
	glm::mat4 mat_p = glm::mat4(1.0f);
};

using Model = DrawModel;
} // namespace ubo
} // namespace vf
