#include <detail/renderer.hpp>
#include <ktl/fixed_vector.hpp>
#include <limits>

namespace vf {
namespace {
vk::UniqueRenderPass makeRenderPass(vk::Device device, vk::Format colour, vk::Format depth) {
	auto attachments = ktl::fixed_vector<vk::AttachmentDescription, 2>{};
	vk::AttachmentReference colourAttachment, depthAttachment;
	auto attachment = vk::AttachmentDescription{};
	attachment.format = colour;
	attachment.samples = vk::SampleCountFlagBits::e1;
	attachment.loadOp = vk::AttachmentLoadOp::eClear;
	attachment.storeOp = vk::AttachmentStoreOp::eStore;
	attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
	attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
	attachment.initialLayout = attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
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

[[maybe_unused]] vk::SurfaceFormatKHR bestColour(vk::PhysicalDevice pd, vk::SurfaceKHR surface) {
	auto const formats = pd.getSurfaceFormatsKHR(surface);
	for (auto const& format : formats) {
		if (format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
			if (format.format == vk::Format::eR8G8B8A8Srgb || format.format == vk::Format::eB8G8R8A8Srgb) { return format; }
		}
	}
	return formats.empty() ? vk::SurfaceFormatKHR() : formats.front();
}

[[maybe_unused]] vk::Format bestDepth(vk::PhysicalDevice pd) {
	static constexpr vk::Format formats[] = {vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint};
	static constexpr auto features = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
	for (auto const format : formats) {
		vk::FormatProperties const props = pd.getFormatProperties(format);
		if ((props.optimalTilingFeatures & features) == features) { return format; }
	}
	return vk::Format::eD16Unorm;
}
} // namespace

FrameSync FrameSync::make(vk::Device device) {
	return {
		device.createSemaphoreUnique({}),
		device.createSemaphoreUnique({}),
		device.createFenceUnique({vk::FenceCreateFlagBits::eSignaled}),
	};
}

Renderer Renderer::make(Vram vram, VKSurface const& surface, std::size_t buffering) {
	if (!surface.device.device || !vram.device || !vram.allocator) { return {}; }
	buffering = std::clamp(buffering, std::size_t(2), surface.swapchain.images.size());
	auto ret = Renderer{vram};
	// TODO: off-screen
	// auto const colour = bestColour(surface.device.gpu.device, surface.surface).format;
	auto const colour = surface.info.imageFormat;
	// TODO
	// auto const depth = bestDepth(surface.device.gpu.device);
	auto const depth = vk::Format();
	ret.renderPass = makeRenderPass(vram.device, colour, depth);

	static constexpr auto flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
	auto cpci = vk::CommandPoolCreateInfo(flags, surface.device.queue.family);
	ret.commandPool = vram.device.createCommandPoolUnique(cpci);
	auto const count = static_cast<std::uint32_t>(buffering);
	auto primaries = vram.device.allocateCommandBuffersUnique({*ret.commandPool, vk::CommandBufferLevel::ePrimary, count});
	auto secondaries = vram.device.allocateCommandBuffersUnique({*ret.commandPool, vk::CommandBufferLevel::eSecondary, count});
	for (std::size_t i = 0; i < buffering; ++i) {
		auto sync = FrameSync::make(vram.device);
		sync.cmd.primary = std::move(primaries[i]);
		sync.cmd.secondary = std::move(secondaries[i]);
		ret.frameSync.push(std::move(sync));
	}
	return ret;
}

vk::CommandBuffer Renderer::beginPass(VKImage const& target) {
	this->target = target;
	auto& s = frameSync.get();
	if (vram.device.getFenceStatus(*s.drawn) == vk::Result::eNotReady) { vram.device.waitForFences(*s.drawn, true, std::numeric_limits<std::uint64_t>::max()); }
	vram.device.resetFences(*s.drawn);
	s.framebuffer = makeFramebuffer({target, {}});
	auto const cbii = vk::CommandBufferInheritanceInfo(*renderPass, 0U, *s.framebuffer);
	s.cmd.secondary->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue, &cbii});
	return *s.cmd.secondary;
}

vk::CommandBuffer Renderer::endPass() {
	auto& s = frameSync.get();
	s.cmd.secondary->end();
	s.cmd.primary->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	auto meta = ImgMeta{};
	meta.access = {{}, vk::AccessFlagBits::eColorAttachmentWrite};
	meta.stages = {vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput};
	meta.layouts = {vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal};
	meta.imageBarrier(*s.cmd.primary, target.image);
	auto const renderArea = vk::Rect2D({}, target.extent);
	vk::ClearValue const clear = vk::ClearColorValue(std::array{0.0f, 0.0f, 0.0f, 0.0f});
	s.cmd.primary->beginRenderPass(vk::RenderPassBeginInfo(*renderPass, *s.framebuffer, renderArea, 1U, &clear), vk::SubpassContents::eSecondaryCommandBuffers);
	s.cmd.primary->executeCommands(*s.cmd.secondary);
	s.cmd.primary->endRenderPass();
	meta.access = {vk::AccessFlagBits::eColorAttachmentWrite, {}};
	meta.stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe};
	meta.layouts = {vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR};
	meta.imageBarrier(*s.cmd.primary, target.image);
	s.cmd.primary->end();
	return *s.cmd.primary;
}

vk::UniqueFramebuffer Renderer::makeFramebuffer(RenderTarget const& target) const {
	if (!target.colour.view) { return {}; }
	auto attachments = ktl::fixed_vector<vk::ImageView, 4>{target.colour.view};
	if (target.depth.view) { attachments.push_back(target.depth.view); }
	auto const extent = target.colour.extent;
	auto fci = vk::FramebufferCreateInfo({}, *renderPass, static_cast<std::uint32_t>(attachments.size()), attachments.data(), extent.width, extent.height, 1U);
	return vram.device.createFramebufferUnique(fci);
}
} // namespace vf
