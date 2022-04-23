#include <detail/renderer.hpp>
#include <ktl/fixed_vector.hpp>

namespace vf {
namespace {
vk::UniqueRenderPass makeRenderPass(vk::Device device, vk::Format colour, vk::Format depth, bool autoTransition) {
	auto attachments = ktl::fixed_vector<vk::AttachmentDescription, 2>{};
	vk::AttachmentReference colourAttachment, depthAttachment;
	auto attachment = vk::AttachmentDescription{};
	attachment.format = colour;
	attachment.samples = vk::SampleCountFlagBits::e1;
	attachment.loadOp = vk::AttachmentLoadOp::eClear;
	attachment.storeOp = vk::AttachmentStoreOp::eStore;
	attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	if (autoTransition) {
		attachment.initialLayout = vk::ImageLayout::eUndefined;
		attachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
	} else {
		attachment.initialLayout = attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
	}
	depthAttachment.attachment = static_cast<std::uint32_t>(attachments.size());
	colourAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
	attachments.push_back(attachment);
	if (depth != vk::Format()) {
		auto attachment = vk::AttachmentDescription{};
		attachment.format = depth;
		attachment.samples = vk::SampleCountFlagBits::e1;
		attachment.loadOp = vk::AttachmentLoadOp::eClear;
		attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
		attachment.stencilLoadOp = vk::AttachmentLoadOp::eClear;
		attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachment.initialLayout = attachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		if (autoTransition) { attachment.initialLayout = vk::ImageLayout::eUndefined; }
		depthAttachment.attachment = static_cast<std::uint32_t>(attachments.size());
		depthAttachment.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		attachments.push_back(attachment);
	}
	vk::SubpassDescription subpass;
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colourAttachment;
	subpass.pDepthStencilAttachment = depth == vk::Format() ? nullptr : &depthAttachment;
	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = static_cast<std::uint32_t>(attachments.size());
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = 1U;
	createInfo.pSubpasses = &subpass;
	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite;
	dependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
	dependency.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	// if (offscreen) {
	// 	ret.srcAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite;
	// 	ret.dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;
	// 	ret.srcStageMask |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
	// }
	dependency.dstStageMask = dependency.srcStageMask;
	createInfo.dependencyCount = 1U;
	createInfo.pDependencies = &dependency;
	return device.createRenderPassUnique(createInfo);
}

vk::SurfaceFormatKHR bestColour(vk::PhysicalDevice pd, vk::SurfaceKHR surface) {
	auto const formats = pd.getSurfaceFormatsKHR(surface);
	for (auto const& format : formats) {
		if (format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
			if (format.format == vk::Format::eR8G8B8A8Srgb || format.format == vk::Format::eB8G8R8A8Srgb) { return format; }
		}
	}
	return formats.empty() ? vk::SurfaceFormatKHR() : formats.front();
}

vk::Format bestDepth(vk::PhysicalDevice pd) {
	static constexpr vk::Format formats[] = {vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint};
	static constexpr auto features = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
	for (auto const format : formats) {
		vk::FormatProperties const props = pd.getFormatProperties(format);
		if ((props.optimalTilingFeatures & features) == features) { return format; }
	}
	return vk::Format::eD16Unorm;
}
} // namespace

Renderer Renderer::make(VKGpu const& gpu, Vram vram, vk::SurfaceKHR surface, bool autoTransition) {
	if (!gpu.device || !vram.device || !vram.allocator) { return {}; }
	auto ret = Renderer{vram};
	ret.renderPass = makeRenderPass(vram.device, bestColour(gpu.device, surface).format, bestDepth(gpu.device), autoTransition);
	return ret;
}
} // namespace vf
