#include <detail/pipeline_factory.hpp>
#include <detail/trace.hpp>
#include <ktl/fixed_vector.hpp>
#include <spir_v/default.frag.hpp>
#include <spir_v/default.vert.hpp>

namespace vf {
namespace {
bool make_default_shaders(vk::Device device, vk::UniqueShaderModule& vert, vk::UniqueShaderModule& frag) {
	vert = device.createShaderModuleUnique({{}, std::size(default_vert_v), reinterpret_cast<std::uint32_t const*>(default_vert_v)});
	frag = device.createShaderModuleUnique({{}, std::size(default_frag_v), reinterpret_cast<std::uint32_t const*>(default_frag_v)});
	return vert && frag;
}
} // namespace

PipelineFactory PipelineFactory::make(VulkanDevice const& device, VertexInput vertex_input, SetLayouts set_layouts, vk::SampleCountFlagBits samples, bool srr) {
	if (!device) { return {}; }
	auto ret = PipelineFactory{};
	if (!make_default_shaders(device.device, ret.default_shaders.vert, ret.default_shaders.frag)) {
		VF_TRACE("vf::(internal)", trace::Type::eError, "Failed to create default shader modules");
		return {};
	}
	ret.device = device.device;
	ret.vertex_input = std::move(vertex_input);
	ret.set_layouts = std::move(set_layouts);
	auto const limits = device.gpu.getProperties().limits;
	ret.line_width_limit = {limits.lineWidthRange[0], limits.lineWidthRange[1]};
	ret.samples = samples;
	ret.sample_rate_shading = srr;
	return ret;
}

PipelineFactory::Entry* PipelineFactory::find(Spec const& spec) {
	auto const it = std::find_if(entries.begin(), entries.end(), [&spec](Entry const& e) { return e.spec == spec; });
	return it == entries.end() ? nullptr : &*it;
}

PipelineFactory::Entry* PipelineFactory::get_or_load(Spec const& spec) {
	if (auto ret = find(spec)) { return ret; }
	if (!device) { return {}; }
	Entry entry{spec};
	entry.layout = device.createPipelineLayoutUnique({{}, static_cast<std::uint32_t>(set_layouts.size()), set_layouts.data()});
	entries.push_back(std::move(entry));
	return &entries.back();
}

vk::PipelineLayout PipelineFactory::layout(Spec const& spec) {
	if (auto entry = get_or_load(spec)) { return *entry->layout; }
	return {};
}

std::pair<vk::Pipeline, vk::PipelineLayout> PipelineFactory::pipeline(Spec spec, vk::RenderPass render_pass) {
	if (!spec.shader.vert) { spec.shader.vert = *default_shaders.vert; }
	if (!spec.shader.frag) { spec.shader.frag = *default_shaders.frag; }
	auto entry = get_or_load(spec);
	if (!entry) { return {}; }
	auto it = entry->pipelines.find(render_pass);
	if (it != entry->pipelines.end()) { return {*it->second, *entry->layout}; }
	auto pipeline = make_pipeline(*entry->layout, std::move(spec), render_pass);
	if (!pipeline) { return {}; }
	auto [i, _] = entry->pipelines.insert_or_assign(render_pass, std::move(pipeline));
	return {*i->second, *entry->layout};
}

vk::UniquePipeline PipelineFactory::make_pipeline(vk::PipelineLayout layout, Spec spec, vk::RenderPass render_pass) const {
	auto gpci = vk::GraphicsPipelineCreateInfo{};
	gpci.renderPass = render_pass;
	gpci.layout = layout;

	auto pvisci = vk::PipelineVertexInputStateCreateInfo{};
	pvisci.pVertexBindingDescriptions = vertex_input.bindings.data();
	pvisci.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertex_input.bindings.size());
	pvisci.pVertexAttributeDescriptions = vertex_input.attributes.data();
	pvisci.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertex_input.attributes.size());
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

	auto pdssci = vk::PipelineDepthStencilStateCreateInfo{};
	pdssci.depthTestEnable = pdssci.depthWriteEnable = spec.depth_test;
	pdssci.depthCompareOp = vk::CompareOp::eLess;
	gpci.pDepthStencilState = &pdssci;

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

	auto pdsci = vk::PipelineDynamicStateCreateInfo();
	vk::DynamicState const pdscis[] = {vk::DynamicState::eViewport, vk::DynamicState::eScissor, vk::DynamicState::eLineWidth};
	pdsci = vk::PipelineDynamicStateCreateInfo({}, static_cast<std::uint32_t>(std::size(pdscis)), pdscis);
	gpci.pDynamicState = &pdsci;

	auto pvsci = vk::PipelineViewportStateCreateInfo({}, 1, {}, 1);
	gpci.pViewportState = &pvsci;

	auto pmssci = vk::PipelineMultisampleStateCreateInfo{};
	pmssci.rasterizationSamples = samples;
	pmssci.sampleShadingEnable = sample_rate_shading;
	gpci.pMultisampleState = &pmssci;

	try {
		return device.createGraphicsPipelineUnique({}, gpci).value;
	} catch ([[maybe_unused]] std::runtime_error const& e) {
		VF_TRACEW("vf::(internal)", "Pipeline creation failure! {}", e.what());
		return {};
	}
}
} // namespace vf
