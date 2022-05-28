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

PipelineFactory PipelineFactory::make(VKDevice const& device, VertexInput vertexInput, SetLayouts setLayouts, vk::SampleCountFlagBits samples, bool srr) {
	if (!device) { return {}; }
	auto ret = PipelineFactory{};
	if (!makeDefaultShaders(device.device, ret.defaultShaders.vert, ret.defaultShaders.frag)) {
		VF_TRACE("[vf::(Internal)] Failed to create default shader modules");
		return {};
	}
	ret.device = device.device;
	ret.vertexInput = std::move(vertexInput);
	ret.setLayouts = std::move(setLayouts);
	auto const limits = device.gpu.getProperties().limits;
	ret.lineWidthLimit = {limits.lineWidthRange[0], limits.lineWidthRange[1]};
	ret.samples = samples;
	ret.sampleRateShading = srr;
	return ret;
}

PipelineFactory::Entry* PipelineFactory::find(Spec const& spec) {
	auto const it = std::find_if(entries.begin(), entries.end(), [&spec](Entry const& e) { return e.spec == spec; });
	return it == entries.end() ? nullptr : &*it;
}

PipelineFactory::Entry* PipelineFactory::getOrLoad(Spec const& spec) {
	if (auto ret = find(spec)) { return ret; }
	if (!device) { return {}; }
	Entry entry{spec};
	entry.layout = device.createPipelineLayoutUnique({{}, static_cast<std::uint32_t>(setLayouts.size()), setLayouts.data()});
	entries.push_back(std::move(entry));
	return &entries.back();
}

vk::PipelineLayout PipelineFactory::layout(Spec const& spec) {
	if (auto entry = getOrLoad(spec)) { return *entry->layout; }
	return {};
}

std::pair<vk::Pipeline, vk::PipelineLayout> PipelineFactory::pipeline(Spec spec, vk::RenderPass renderPass) {
	if (!spec.shader.vert) { spec.shader.vert = *defaultShaders.vert; }
	if (!spec.shader.frag) { spec.shader.frag = *defaultShaders.frag; }
	auto entry = getOrLoad(spec);
	if (!entry) { return {}; }
	auto it = entry->pipelines.find(renderPass);
	if (it != entry->pipelines.end()) { return {*it->second, *entry->layout}; }
	auto pipeline = makePipeline(*entry->layout, std::move(spec), renderPass);
	if (!pipeline) { return {}; }
	auto [i, _] = entry->pipelines.insert_or_assign(renderPass, std::move(pipeline));
	return {*i->second, *entry->layout};
}

vk::UniquePipeline PipelineFactory::makePipeline(vk::PipelineLayout layout, Spec spec, vk::RenderPass renderPass) const {
	auto gpci = vk::GraphicsPipelineCreateInfo{};
	gpci.renderPass = renderPass;
	gpci.layout = layout;

	auto pvisci = vk::PipelineVertexInputStateCreateInfo{};
	pvisci.pVertexBindingDescriptions = vertexInput.bindings.data();
	pvisci.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertexInput.bindings.size());
	pvisci.pVertexAttributeDescriptions = vertexInput.attributes.data();
	pvisci.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertexInput.attributes.size());
	gpci.pVertexInputState = &pvisci;

	auto pssciVert = vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, spec.shader.vert, "main");
	auto pssciFrag = vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, spec.shader.frag, "main");
	auto psscis = std::array<vk::PipelineShaderStageCreateInfo, 2>{{pssciVert, pssciFrag}};
	gpci.stageCount = static_cast<std::uint32_t>(psscis.size());
	gpci.pStages = psscis.data();

	auto piasci = vk::PipelineInputAssemblyStateCreateInfo({}, spec.topology);
	gpci.pInputAssemblyState = &piasci;

	auto prsci = vk::PipelineRasterizationStateCreateInfo();
	prsci.polygonMode = spec.mode;
	prsci.cullMode = vk::CullModeFlagBits::eNone;
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

	auto pdscis = ktl::fixed_vector<vk::DynamicState, 4>{};
	auto pdsci = vk::PipelineDynamicStateCreateInfo();
	pdscis = {vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eLineWidth};
	pdsci = vk::PipelineDynamicStateCreateInfo({}, static_cast<std::uint32_t>(pdscis.size()), pdscis.data());
	gpci.pDynamicState = &pdsci;

	auto pvsci = vk::PipelineViewportStateCreateInfo({}, 1, {}, 1);
	gpci.pViewportState = &pvsci;

	auto pmssci = vk::PipelineMultisampleStateCreateInfo{};
	pmssci.rasterizationSamples = samples;
	pmssci.sampleShadingEnable = sampleRateShading;
	gpci.pMultisampleState = &pmssci;

	try {
		return device.createGraphicsPipelineUnique({}, gpci).value;
	} catch ([[maybe_unused]] std::runtime_error const& e) {
		VF_TRACEF("[vf::(Internal)] Pipeline creation failure! {}", e.what());
		return {};
	}
}
} // namespace vf
