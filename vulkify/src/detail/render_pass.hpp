#pragma once
#include <detail/cache.hpp>
#include <detail/set_writer.hpp>
#include <glm/vec2.hpp>
#include <ktl/unique_val.hpp>
#include <vulkify/graphics/camera.hpp>
#include <vulkify/graphics/resources/texture_handle.hpp>
#include <mutex>

namespace vf {
class Instance;
struct DrawModel;
struct PipelineFactory;
struct DescriptorSetFactory;
struct HTexture;

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

	SetWriter mat_p{};
	Textures const* textures{};
	SetBind one{1};
	SetBind two{2};
};

struct RenderCam {
	glm::vec2 extent{};
	Camera const* camera{};
};

struct RenderPass {
	ktl::unique_val<Instance*> instance{};
	PipelineFactory* pipelineFactory{};
	DescriptorSetFactory* setFactory{};
	vk::RenderPass renderPass{};
	vk::CommandBuffer commandBuffer{};
	ShaderInput shaderInput{};
	RenderCam cam{};
	TPair<float> lineWidthLimit{};
	std::mutex* renderMutex;

	mutable vk::PipelineLayout bound{};

	HTexture whiteTexture() const;
	void writeView(SetWriter& set) const;
	void writeModels(SetWriter& set, std::span<DrawModel const> instances, TextureHandle const& texture) const;
	void writeCustom(SetWriter& set, std::span<std::byte const> ubo, TextureHandle const& texture) const;
	void bind(vk::PipelineLayout layout, vk::Pipeline pipeline) const;
	void setViewport() const;
};
} // namespace vf
