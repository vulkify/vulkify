#pragma once
#include <detail/descriptor_set.hpp>
#include <detail/vram.hpp>
#include <glm/mat4x4.hpp>
#include <ktl/unique_val.hpp>
#include <vulkify/core/rgba.hpp>

namespace vf {
struct PipelineFactory;
class Instance;
struct DrawParams;

struct SetBind {
	std::uint32_t set{};
	std::uint32_t binding{};
};

struct ShaderInput {
	DescriptorSet setZero{};
	SetBind modelMat{};
};

struct RenderPass {
	ktl::unique_val<Instance*> instance{};
	PipelineFactory* pipelineFactory{};
	DescriptorPool* descriptorPool{};
	vk::RenderPass renderPass{};
	vk::CommandBuffer commandBuffer{};
	ShaderInput mat{};
	Rgba* clear{};

	mutable vk::PipelineLayout bound{};

	void writeDrawParams(DrawParams const& params) const;
};

struct GfxResource {
	Vram vram{};
	BufferObject buffer{};
};

namespace ubo {
struct View {
	glm::mat4 mat_vp = glm::mat4(1.0f);
};

struct Model {
	glm::mat4 mat_m = glm::mat4(1.0f);
	glm::vec4 tint = white_v.normalize();
};
} // namespace ubo
} // namespace vf
