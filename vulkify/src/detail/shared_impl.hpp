#pragma once
#include <detail/descriptor_set.hpp>
#include <detail/image_cache.hpp>
#include <detail/vram.hpp>
#include <glm/mat4x4.hpp>
#include <ktl/unique_val.hpp>
#include <vulkify/graphics/draw_model.hpp>

namespace vf {
struct PipelineFactory;
class Instance;
struct DrawModel;
class Texture;
struct View;

inline vk::SamplerCreateInfo samplerInfo(Vram const& vram, vk::SamplerAddressMode mode, vk::Filter filter) {
	auto ret = vk::SamplerCreateInfo{};
	ret.minFilter = ret.magFilter = filter;
	ret.anisotropyEnable = vram.maxAnisotropy > 0.0f;
	ret.maxAnisotropy = vram.maxAnisotropy;
	ret.borderColor = vk::BorderColor::eIntOpaqueBlack;
	ret.mipmapMode = vk::SamplerMipmapMode::eNearest;
	ret.addressModeU = ret.addressModeV = ret.addressModeW = mode;
	return ret;
}

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
	Textures* textures{};
	SetBind one{1};
	SetBind two{2};
};

struct RenderView {
	glm::uvec2 extent{};
	View const* view{};
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
	RenderView view{};
	TPair<float> lineWidthLimit{};

	mutable vk::ShaderModule fragShader{};
	mutable vk::PipelineLayout bound{};

	void writeView(DescriptorSet& set) const;
	void writeModels(DescriptorSet& set, std::span<DrawModel const> instances, Tex tex) const;
	void bind(vk::PipelineLayout layout, vk::Pipeline pipeline) const;
	void setViewport() const;
};

struct ImageSampler {
	ImageCache cache{};
	vk::UniqueSampler sampler{};

	bool operator==(ImageSampler const& rhs) const { return !sampler && !rhs.sampler && !cache.image && !rhs.cache.image; }
};

struct GfxAllocation {
	ImageSampler image{};
	BufferCache buffer{};
	Vram vram{};
	std::string name{};

	GfxAllocation() = default;
	GfxAllocation(Vram const& vram, std::string name) : image{{{vram, name}}}, vram(vram), name(std::move(name)) {}

	bool operator==(GfxAllocation const& rhs) const { return !image.sampler && !rhs.image.sampler; }

	void replace(ImageCache&& cache) {
		vram.device.defer(std::move(image.cache));
		image.cache = std::move(cache);
	}
};

struct GfxShaderModule {
	vk::UniqueShaderModule module{};
	vk::Device device{};
};

struct Inactive {
	Vram vram{};
	GfxAllocation alloc{};
	std::string name{};
};

Inactive const g_inactive{};
} // namespace vf
