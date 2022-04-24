#include <detail/pipeline_factory.hpp>
#include <detail/trace.hpp>
#include <detail/vk_instance.hpp>
#include <ktl/fixed_vector.hpp>
#include <filesystem>

namespace vf {
namespace stdfs = std::filesystem;

namespace {
std::string spirvPath(std::string_view const glsl) { return std::string(glsl) + ".spv"; }

std::pair<SpirV, std::string> loadOrCompile(std::string path) {
	if (ShaderCache::isGlsl(path)) {
		static bool s_glslc = SpirV::glslcAvailable();
		auto spv = std::string{};
		if (s_glslc) {
			// try-compile Glsl to Spir-V
			spv = spirvPath(path);
			auto ret = SpirV::compile(path.c_str(), spv);
			if (ret.bytes) { return {std::move(ret), std::move(spv)}; }
		}
		// search for pre-compiled Spir-V
		path = spv.empty() ? spirvPath(path) : std::move(spv);
		if (!stdfs::is_regular_file(path)) { return {}; }
	}
	// load Spir-V
	auto ret = SpirV::load(path);
	if (!ret.bytes) { return {}; }
	return {std::move(ret), std::move(path)};
}
} // namespace

bool ShaderCache::isGlsl(std::string_view path) {
	auto p = stdfs::path(path);
	auto const ext = p.extension().string();
	return ext == ".vert" || ext == ".frag";
}

bool ShaderCache::load(std::string path, bool force) {
	if (map.contains(path) && !force) { return true; }
	if (!device) { return false; }
	auto [spirv, key] = loadOrCompile(std::move(path));
	if (!spirv.bytes) { return false; }
	auto module = device.createShaderModuleUnique(vk::ShaderModuleCreateInfo({}, spirv.codesize, spirv.bytes.get()));
	if (!module) { return false; }
	map.insert_or_assign(std::move(key), std::move(module));
	return true;
}

PipelineFactory PipelineFactory::make(std::string vertPath, VKDevice const& device, VertexInput vertexInput, SetLayouts setLayouts) {
	if (!device.device || !device.gpu.device) { return {}; }
	auto ret = PipelineFactory{};
	ret.cache.device = device.device;
	if (!ret.cache.load(vertPath)) { return {}; }
	ret.vertPath = std::move(vertPath);
	ret.lineWidthLimit = {device.gpu.properties.limits.lineWidthRange[0], device.gpu.properties.limits.lineWidthRange[1]};
	ret.vertexInput = std::move(vertexInput);
	ret.setLayouts = std::move(setLayouts);
	return ret;
}

PipelineFactory::Entry* PipelineFactory::find(PipelineSpec const& spec) {
	auto const it = std::find_if(entries.begin(), entries.end(), [&spec](Entry const& e) { return e.spec == spec; });
	return it == entries.end() ? nullptr : &*it;
}

PipelineFactory::Entry* PipelineFactory::getOrLoad(PipelineSpec const& spec) {
	if (auto ret = find(spec)) { return ret; }
	// if (setLayouts.empty()) { return {}; }
	Entry entry{spec};
	entry.layout = cache.device.createPipelineLayoutUnique({{}, static_cast<std::uint32_t>(setLayouts.size()), setLayouts.data()});
	entries.push_back(std::move(entry));
	return &entries.back();
}

vk::Pipeline PipelineFactory::get(PipelineSpec const& spec, vk::RenderPass renderPass) {
	auto entry = getOrLoad(spec);
	if (!entry) { return {}; }
	auto it = entry->pipelines.find(renderPass);
	if (it != entry->pipelines.end()) { return *it->second; }
	auto vert = cache.getOrLoad(vertPath);
	if (!vert) { return {}; }
	auto frag = cache.getOrLoad(spec.spirvPath);
	if (!frag) { return {}; }
	auto pipeline = makePipeline(spec, *entry->layout, {vert, frag}, renderPass);
	if (!pipeline) { return {}; }
	auto [i, _] = entry->pipelines.insert_or_assign(renderPass, std::move(pipeline));
	return *i->second;
}

vk::UniquePipeline PipelineFactory::makePipeline(PipelineSpec spec, vk::PipelineLayout pipelineLayout, Shaders shaders, vk::RenderPass renderPass) const {
	auto gpci = vk::GraphicsPipelineCreateInfo{};
	gpci.renderPass = renderPass;
	gpci.layout = pipelineLayout;

	auto pvisci = vk::PipelineVertexInputStateCreateInfo{};
	pvisci.pVertexBindingDescriptions = vertexInput.bindings.data();
	pvisci.vertexBindingDescriptionCount = static_cast<std::uint32_t>(vertexInput.bindings.size());
	pvisci.pVertexAttributeDescriptions = vertexInput.attributes.data();
	pvisci.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(vertexInput.attributes.size());
	gpci.pVertexInputState = &pvisci;

	auto pssciVert = vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, shaders.first, "main");
	auto pssciFrag = vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, shaders.second, "main");
	auto psscis = std::array<vk::PipelineShaderStageCreateInfo, 2>{{pssciVert, pssciFrag}};
	gpci.stageCount = static_cast<std::uint32_t>(psscis.size());
	gpci.pStages = psscis.data();

	auto piasci = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList);
	gpci.pInputAssemblyState = &piasci;

	auto prsci = vk::PipelineRasterizationStateCreateInfo();
	prsci.lineWidth = std::clamp(spec.lineWidth, lineWidthLimit.first, lineWidthLimit.second);
	prsci.polygonMode = vk::PolygonMode::eFill;
	if (spec.flags.test(PipelineFlag::eWireframe)) { prsci.polygonMode = vk::PolygonMode::eLine; }
	gpci.pRasterizationState = &prsci;

	auto pcbas = vk::PipelineColorBlendAttachmentState{};
	if (!spec.flags.test(PipelineFlag::eNoAlphaBlend)) {
		using CCF = vk::ColorComponentFlagBits;
		pcbas.colorWriteMask = CCF::eR | CCF::eG | CCF::eB | CCF::eA;
		pcbas.blendEnable = true;
		pcbas.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		pcbas.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		pcbas.colorBlendOp = vk::BlendOp::eAdd;
		pcbas.srcAlphaBlendFactor = vk::BlendFactor::eOne;
		pcbas.dstAlphaBlendFactor = vk::BlendFactor::eZero;
		pcbas.alphaBlendOp = vk::BlendOp::eAdd;
	}
	auto pcbsci = vk::PipelineColorBlendStateCreateInfo();
	pcbsci.attachmentCount = 1;
	pcbsci.pAttachments = &pcbas;
	gpci.pColorBlendState = &pcbsci;

	auto pdssci = vk::PipelineDepthStencilStateCreateInfo{};
	if (!spec.flags.test(PipelineFlag::eNoDepthTest)) {
		pdssci.depthTestEnable = true;
		pdssci.depthCompareOp = vk::CompareOp::eLess;
	}
	if (!spec.flags.test(PipelineFlag::eNoDepthWrite)) { pdssci.depthWriteEnable = true; }
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
		return cache.device.createGraphicsPipelineUnique({}, gpci);
	} catch ([[maybe_unused]] std::runtime_error const& e) {
		VF_TRACEF("Pipeline creation failure! {}", e.what());
		return {};
	}
}
} // namespace vf