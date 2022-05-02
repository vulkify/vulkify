#pragma once
#include <ktl/hash_table.hpp>
#include <vulkan/vulkan.hpp>
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

struct PipelineFactory {
	struct Shaders {
		std::string vert{};
		std::string frag{};

		bool operator==(Shaders const&) const = default;
	};

	struct Entry {
		Shaders shaders{};
		vk::UniquePipelineLayout layout{};
		ktl::hash_table<vk::RenderPass, vk::UniquePipeline> pipelines{};
	};
	using ShaderProgram = std::pair<vk::ShaderModule, vk::ShaderModule>;
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

	Entry* find(Shaders const& shaders);
	Entry* getOrLoad(Shaders const& shaders);

	std::pair<vk::Pipeline, vk::PipelineLayout> pipeline(Shaders const& shaders, vk::RenderPass renderPass);
	vk::PipelineLayout layout(Shaders const& shaders);

	vk::UniquePipelineLayout makeLayout() const;
	vk::UniquePipeline makePipeline(vk::PipelineLayout layout, ShaderProgram shader, vk::RenderPass renderPass) const;
};
} // namespace vf
