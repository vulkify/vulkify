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
	auto const depth = bestDepth(surface.device.gpu.device);
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
	ret.depthImage = {vram};
	ret.depthImage.setDepth();
	ret.depthImage.info.format = depth;
	return ret;
}

vk::CommandBuffer Renderer::beginPass(VKImage const& target) {
	attachments.colour = target;
	auto& s = frameSync.get();
	if (vram.device.getFenceStatus(*s.drawn) == vk::Result::eNotReady) { vram.device.waitForFences(*s.drawn, true, std::numeric_limits<std::uint64_t>::max()); }
	vram.device.resetFences(*s.drawn);
	attachments.depth = depthImage.refresh({target.extent.width, target.extent.height, 1}, depthImage.info.format);
	s.framebuffer = makeFramebuffer(attachments);
	auto const cbii = vk::CommandBufferInheritanceInfo(*renderPass, 0U, *s.framebuffer);
	s.cmd.secondary->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue, &cbii});
	return *s.cmd.secondary;
}

vk::CommandBuffer Renderer::endPass(Rgba clear) {
	auto& s = frameSync.get();
	s.cmd.secondary->end();
	s.cmd.primary->begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	auto barrier = ImageBarrier{};
	barrier.access = {{}, vk::AccessFlagBits::eColorAttachmentWrite};
	barrier.stages = {vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput};
	barrier.layouts = {vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal};
	barrier(*s.cmd.primary, attachments.colour.image);
	barrier.access = {{}, vk::AccessFlagBits::eDepthStencilAttachmentWrite};
	barrier.stages.first = barrier.stages.second = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	barrier.layouts.second = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	barrier.aspects = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	barrier(*s.cmd.primary, depthImage.peek().image);
	auto const renderArea = vk::Rect2D({}, attachments.colour.extent);
	auto const c = clear.normalize();
	vk::ClearValue const cvs[] = {vk::ClearColorValue(std::array{c.x, c.y, c.z, c.w}), vk::ClearDepthStencilValue(1.0f)};
	s.cmd.primary->beginRenderPass({*renderPass, *s.framebuffer, renderArea, std::size(cvs), cvs}, vk::SubpassContents::eSecondaryCommandBuffers);
	s.cmd.primary->executeCommands(*s.cmd.secondary);
	s.cmd.primary->endRenderPass();
	barrier.access = {vk::AccessFlagBits::eColorAttachmentWrite, {}};
	barrier.stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe};
	barrier.layouts = {vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR};
	barrier.aspects = vk::ImageAspectFlagBits::eColor;
	barrier(*s.cmd.primary, attachments.colour.image);
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