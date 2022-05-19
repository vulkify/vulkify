#pragma once
#include <detail/cache.hpp>
#include <detail/descriptor_set.hpp>
#include <glm/vec2.hpp>
#include <ktl/unique_val.hpp>
#include <vulkify/graphics/render_view.hpp>

namespace vf {
class Instance;
struct DrawModel;
struct PipelineFactory;

struct SetBind {
	std::uint32_t set{};
	struct {
		std::uint32_t ubo{0};
		std::uint32_t ssbo{1};
		std::uint32_t sampler{2};
	} bindings{};
};

struct ShaderInput {
	struct Textures {
		vk::UniqueSampler sampler{};
		ImageCache white{};
		ImageCache magenta{};

		explicit operator bool() const { return sampler && white.image && magenta.image; }
	};

	DescriptorSet mat_p{};
	Textures const* textures{};
	SetBind one{1};
	SetBind two{2};
};

struct RenderPassView {
	glm::uvec2 extent{};
	RenderView const* view{};
};

struct RenderPass {
	struct Tex {
		vk::Sampler sampler{};
		vk::ImageView view{};
	};

	ktl::unique_val<Instance*> instance{};
	PipelineFactory* pipelineFactory{};
	DescriptorPool* descriptorPool{};
	vk::RenderPass renderPass{};
	vk::CommandBuffer commandBuffer{};
	ShaderInput shaderInput{};
	RenderPassView view{};
	TPair<float> lineWidthLimit{};
	std::thread::id renderThread{};

	mutable vk::ShaderModule fragShader{};
	mutable vk::PipelineLayout bound{};

	void writeView(DescriptorSet& set) const;
	void writeModels(DescriptorSet& set, std::span<DrawModel const> instances, Tex tex) const;
	void bind(vk::PipelineLayout layout, vk::Pipeline pipeline) const;
	void setViewport() const;
};
} // namespace vf