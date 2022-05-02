#pragma once
#include <ktl/hash_table.hpp>
#include <vulkan/vulkan.hpp>
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
	struct ShaderProgram {
		vk::ShaderModule vert{};
		vk::ShaderModule frag{};

		bool operator==(ShaderProgram const&) const = default;
	};

	struct Entry {
		ShaderProgram shader{};
		vk::UniquePipelineLayout layout{};
		ktl::hash_table<vk::RenderPass, vk::UniquePipeline> pipelines{};
	};
	using SetLayouts = std::vector<vk::DescriptorSetLayout>;

	vk::Device device{};
	VertexInput vertexInput{};
	SetLayouts setLayouts{};
	std::vector<Entry> entries{};
	struct {
		vk::UniqueShaderModule vert{};
		vk::UniqueShaderModule frag{};
	} defaultShaders{};

	static PipelineFactory make(vk::Device const& device, VertexInput vertexInput, SetLayouts setLayouts);

	explicit operator bool() const { return device; }

	Entry* find(ShaderProgram const& shaders);
	Entry* getOrLoad(ShaderProgram const& shaders);

	std::pair<vk::Pipeline, vk::PipelineLayout> pipeline(ShaderProgram shaders, vk::RenderPass renderPass);
	vk::PipelineLayout layout(ShaderProgram const& shaders);

	vk::UniquePipelineLayout makeLayout() const;
	vk::UniquePipeline makePipeline(vk::PipelineLayout layout, ShaderProgram shader, vk::RenderPass renderPass) const;
};
} // namespace vf
