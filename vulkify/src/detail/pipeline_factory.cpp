#include <detail/pipeline_factory.hpp>
#include <detail/trace.hpp>
#include <detail/vk_device.hpp>
#include <ktl/fixed_vector.hpp>
#include <spir_v/default.frag.hpp>
#include <spir_v/default.vert.hpp>

namespace vf {
namespace {
bool makeDefaultShaders(vk::Device device, vk::UniqueShaderModule& vert, vk::UniqueShaderModule& frag) {
	vert = device.createShaderModuleUnique({{}, std::size(default_vert_v), reinterpret_cast<std::uint32_t const*>(default_vert_v)});
	frag = device.createShaderModuleUnique({{}, std::size(default_frag_v), reinterpret_cast<std::uint32_t const*>(default_frag_v)});
	return vert && frag;
}
} // namespace

PipelineFactory PipelineFactory::make(vk::Device const& device, VertexInput vertexInput, SetLayouts setLayouts) {
	if (!device) { return {}; }
	auto ret = PipelineFactory{};
	if (!makeDefaultShaders(device, ret.defaultShaders.vert, ret.defaultShaders.frag)) {
		VF_TRACE("Failed to create default shader modules");
		return {};
	}
	ret.device = device;
	ret.vertexInput = std::move(vertexInput);
	ret.setLayouts = std::move(setLayouts);
	return ret;
}

PipelineFactory::Entry* PipelineFactory::find(ShaderProgram const& shaders) {
	auto const it = std::find_if(entries.begin(), entries.end(), [&shaders](Entry const& e) { return e.shader == shaders; });
	return it == entries.end() ? nullptr : &*it;
}

PipelineFactory::Entry* PipelineFactory::getOrLoad(ShaderProgram const& shaders) {
	if (auto ret = find(shaders)) { return ret; }
	if (!device) { return {}; }
	Entry entry{shaders};
	entry.layout = device.createPipelineLayoutUnique({{}, static_cast<std::uint32_t>(setLayouts.size()), setLayouts.data()});
	entries.push_back(std::move(entry));
	return &entries.back();
}

vk::PipelineLayout PipelineFactory::layout(ShaderProgram const& shaders) {
	if (auto entry = getOrLoad(shaders)) { return *entry->layout; }
	return {};
}

std::pair<vk::Pipeline, vk::PipelineLayout> PipelineFactory::pipeline(ShaderProgram shaders, vk::RenderPass renderPass) {
	if (!shaders.vert) { shaders.vert = *defaultShaders.vert; }
	if (!shaders.frag) { shaders.frag = *defaultShaders.frag; }
	auto entry = getOrLoad(shaders);
	if (!entry) { return {}; }
	auto it = entry->pipelines.find(renderPass);
	if (it != entry->pipelines.end()) { return {*it->second, *entry->layout}; }
	auto pipeline = makePipeline(*entry->layout, shaders, renderPass);
	if (!pipeline) { return {}; }
	auto [i, _] = entry->pipelines.insert_or_assign(renderPass, std::move(pipeline));
	return {*i->second, *entry->layout};
}

vk::UniquePipeline PipelineFactory::makePipeline(vk::PipelineLayout layout, ShaderProgram shader, vk::RenderPass renderPass) const {
	auto gpci = vk::GraphicsPipelineCreateInfo{};
	gpci.renderPass = renderPass;
	gpci.layout = layout;

	auto pvisci = vk::PipelineVertexInputStateCreateInfo{};
	pvisci.pVertexBindingDescriptions = vertexInput.bindings.data();
	pvisci.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertexInput.bindings.size());
	pvisci.pVertexAttributeDescriptions = vertexInput.attributes.data();
	pvisci.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertexInput.attributes.size());
	gpci.pVertexInputState = &pvisci;

	auto pssciVert = vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, shader.vert, "main");
	auto pssciFrag = vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, shader.frag, "main");
	auto psscis = std::array<vk::PipelineShaderStageCreateInfo, 2>{{pssciVert, pssciFrag}};
	gpci.stageCount = static_cast<std::uint32_t>(psscis.size());
	gpci.pStages = psscis.data();

	auto piasci = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList);
	gpci.pInputAssemblyState = &piasci;

	auto prsci = vk::PipelineRasterizationStateCreateInfo();
	prsci.lineWidth = 1.0f;
	// prsci.polygonMode = vk::PolygonMode::eLine;
	gpci.pRasterizationState = &prsci;

	auto pcbas = vk::PipelineColorBlendAttachmentState{};
	using CCF = vk::ColorComponentFlagBits;
	pcbas.colorWriteMask = CCF::eR | CCF::eG | CCF::eB | CCF::eA;
	pcbas.blendEnable = true;
	pcbas.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
	pcbas.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
	pcbas.colorBlendOp = vk::BlendOp::eAdd;
	pcbas.srcAlphaBlendFactor = vk::BlendFactor::eOne;
	pcbas.dstAlphaBlendFactor = vk::BlendFactor::eZero;
	pcbas.alphaBlendOp = vk::BlendOp::eAdd;
	auto pcbsci = vk::PipelineColorBlendStateCreateInfo();
	pcbsci.attachmentCount = 1;
	pcbsci.pAttachments = &pcbas;
	gpci.pColorBlendState = &pcbsci;

	auto pdssci = vk::PipelineDepthStencilStateCreateInfo{};
	pdssci.depthTestEnable = true;
	pdssci.depthCompareOp = vk::CompareOp::eLess;
	pdssci.depthWriteEnable = true;
	gpci.pDepthStencilState = &pdssci;

	auto pdscis = ktl::fixed_vector<vk::DynamicState, 4>{};
	auto pdsci = vk::PipelineDynamicStateCreateInfo();
	pdscis = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	pdsci = vk::PipelineDynamicStateCreateInfo({}, static_cast<std::uint32_t>(pdscis.size()), pdscis.data());
	gpci.pDynamicState = &pdsci;

	auto pvsci = vk::PipelineViewportStateCreateInfo({}, 1, {}, 1);
	gpci.pViewportState = &pvsci;

	auto pmssci = vk::PipelineMultisampleStateCreateInfo{};
	gpci.pMultisampleState = &pmssci;

	try {
		return device.createGraphicsPipelineUnique({}, gpci);
	} catch ([[maybe_unused]] std::runtime_error const& e) {
		VF_TRACEF("Pipeline creation failure! {}", e.what());
		return {};
	}
}
} // namespace vf
