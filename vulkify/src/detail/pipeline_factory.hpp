#pragma once
#include <ktl/hash_table.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkify/graphics/pipeline.hpp>
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
		if (path.empty()) { return {}; }
		if (auto ret = find(path)) { return ret; }
		load(path);
		return find(path);
	}
};

struct VertexInput {
	std::span<vk::VertexInputBindingDescription const> bindings{};
	std::span<vk::VertexInputAttributeDescription const> attributes{};
};

struct VKPipeline {
	vk::Pipeline pipeline{};
	vk::PipelineLayout layout{};
};

struct PipelineSpec {
	Pipeline pipeline{};
	struct Shaders {
		std::string vert{};
		std::string frag{};

		bool operator==(Shaders const&) const = default;
	} shaders{};

	bool operator==(PipelineSpec const&) const = default;
};

struct PipelineFactory {
	struct Entry {
		PipelineSpec spec{};
		vk::UniquePipelineLayout layout{};
		ktl::hash_table<vk::RenderPass, vk::UniquePipeline> pipelines{};
	};
	using Shaders = std::pair<vk::ShaderModule, vk::ShaderModule>;
	using SetLayouts = std::vector<vk::DescriptorSetLayout>;

	ShaderCache cache{};
	VertexInput vertexInput{};
	SetLayouts setLayouts{};
	std::vector<Entry> entries{};
	std::pair<float, float> lineWidthLimit{};
	struct {
		vk::UniqueShaderModule vert{};
		vk::UniqueShaderModule frag{};
	} defaultShaders{};

	static PipelineFactory make(VKDevice const& device, VertexInput vertexInput, SetLayouts setLayouts);

	explicit operator bool() const { return cache.device; }

	Entry* find(PipelineSpec const& spec);
	Entry* getOrLoad(PipelineSpec const& spec);

	std::pair<vk::Pipeline, vk::PipelineLayout> pipeline(PipelineSpec const& spec, vk::RenderPass renderPass);
	vk::PipelineLayout layout(PipelineSpec const& spec);

	vk::UniquePipelineLayout makeLayout() const;
	vk::UniquePipeline makePipeline(PipelineSpec const& spec, vk::PipelineLayout layout, Shaders shaders, vk::RenderPass renderPass) const;
};
} // namespace vf
