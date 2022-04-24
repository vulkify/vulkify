#pragma once
#include <ktl/hash_table.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkify/pipeline/pipeline.hpp>
#include <span>

namespace vf {
struct VKDevice;

struct ShaderCache {
	ktl::hash_table<std::string, vk::UniqueShaderModule> map{};
	vk::Device device{};

	static bool isGlsl(std::string_view path);

	bool load(std::string path, bool force = false);

	vk::ShaderModule find(std::string const& path) const {
		if (auto it = map.find(path); it != map.end()) { return *it->second; }
		return {};
	}

	vk::ShaderModule getOrLoad(std::string const& path) {
		if (auto ret = find(path)) { return ret; }
		load(path);
		return find(path);
	}
};

struct VertexInput {
	std::span<vk::VertexInputBindingDescription> bindings{};
	std::span<vk::VertexInputAttributeDescription> attributes{};
};

struct PipelineFactory {
	struct Entry {
		PipelineSpec spec{};
		vk::UniquePipelineLayout layout{};
		ktl::hash_table<vk::RenderPass, vk::UniquePipeline> pipelines{};
	};
	using Shaders = std::pair<vk::ShaderModule, vk::ShaderModule>;
	using SetLayouts = std::vector<vk::DescriptorSetLayout>;

	std::string vertPath{};
	ShaderCache cache{};
	VertexInput vertexInput{};
	SetLayouts setLayouts{};
	std::vector<Entry> entries{};
	std::pair<float, float> lineWidthLimit{};

	static PipelineFactory make(std::string vertPath, VKDevice const& device, VertexInput vertexInput, SetLayouts setLayouts);

	Entry* find(PipelineSpec const& spec);
	Entry* getOrLoad(PipelineSpec const& spec);

	vk::Pipeline get(PipelineSpec const& spec, vk::RenderPass renderPass);

	vk::UniquePipelineLayout makeLayout() const;
	vk::UniquePipeline makePipeline(PipelineSpec spec, vk::PipelineLayout pipelineLayout, Shaders shaders, vk::RenderPass renderPass) const;
};
} // namespace vf