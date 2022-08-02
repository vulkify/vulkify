#pragma once
#include <detail/vk_instance.hpp>
#include <ktl/hash_table.hpp>
#include <vulkan/vulkan_hash.hpp>
#include <span>

namespace vf {
struct VertexInput {
	std::span<vk::VertexInputBindingDescription const> bindings{};
	std::span<vk::VertexInputAttributeDescription const> attributes{};
};

struct VKPipeline {
	vk::Pipeline pipeline{};
	vk::PipelineLayout layout{};
};

struct PipelineFactory {
	struct Spec {
		struct ShaderProgram {
			vk::ShaderModule vert{};
			vk::ShaderModule frag{};

			bool operator==(ShaderProgram const&) const = default;
		};

		ShaderProgram shader{};
		vk::PolygonMode mode{vk::PolygonMode::eFill};
		vk::PrimitiveTopology topology{vk::PrimitiveTopology::eTriangleList};

		bool operator==(Spec const&) const = default;
	};

	struct Entry {
		Spec spec{};
		vk::UniquePipelineLayout layout{};
		ktl::hash_table<vk::RenderPass, vk::UniquePipeline> pipelines{};
	};
	using SetLayouts = std::vector<vk::DescriptorSetLayout>;

	vk::Device device{};
	VertexInput vertex_input{};
	SetLayouts set_layouts{};
	vk::SampleCountFlagBits samples{};
	bool sample_rate_shading{};

	std::vector<Entry> entries{};
	struct {
		vk::UniqueShaderModule vert{};
		vk::UniqueShaderModule frag{};
	} default_shaders{};
	std::pair<float, float> line_width_limit{1.0f, 1.0f};

	static PipelineFactory make(VKDevice const& device, VertexInput vertexInput, SetLayouts setLayouts, vk::SampleCountFlagBits samples, bool srr);

	explicit operator bool() const { return device; }

	Entry* find(Spec const& spec);
	Entry* get_or_load(Spec const& spec);

	std::pair<vk::Pipeline, vk::PipelineLayout> pipeline(Spec spec, vk::RenderPass renderPass);
	vk::PipelineLayout layout(Spec const& spec);

	vk::UniquePipelineLayout make_layout() const;
	vk::UniquePipeline make_pipeline(vk::PipelineLayout layout, Spec spec, vk::RenderPass renderPass) const;
};
} // namespace vf
