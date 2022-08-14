#pragma once
#include <detail/gfx_allocations.hpp>
#include <detail/set_writer.hpp>
#include <glm/vec2.hpp>
#include <ktl/unique_val.hpp>
#include <vulkify/graphics/camera.hpp>
#include <vulkify/graphics/handle.hpp>
#include <mutex>

namespace vf {
class Instance;
class Texture;
struct DrawModel;
struct PipelineFactory;
struct CombinedImageSampler;
struct DescriptorSetFactory;

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
	Camera* camera{};
};

struct RenderPass {
	ktl::unique_val<Instance*> instance{};
	GfxDevice const* device{};
	PipelineFactory* pipeline_factory{};
	DescriptorSetFactory* set_factory{};
	vk::RenderPass render_pass{};
	vk::CommandBuffer command_buffer{};
	ShaderInput shader_input{};
	RenderCam cam{};
	TPair<float> line_width_limit{};
	std::mutex* render_mutex;

	mutable vk::PipelineLayout bound{};

	CombinedImageSampler image_sampler(Handle<Texture> texture) const;
	CombinedImageSampler white_texture() const;
	void write_view(SetWriter& set) const;
	void write_models(SetWriter& set, std::span<DrawModel const> instances, Handle<Texture> texture) const;
	void write_custom(SetWriter& set, std::span<std::byte const> ubo, Handle<Texture> texture) const;
	void bind(vk::PipelineLayout layout, vk::Pipeline pipeline) const;
	void set_viewport() const;
};
} // namespace vf
