#include <detail/renderer.hpp>
#include <detail/vram.hpp>

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
	dependency.srcAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite;
	dependency.dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;
	dependency.srcStageMask |= vk::PipelineStageFlagBits::eColorAttachmentOutput;
	// }
	dependency.dstStageMask = dependency.srcStageMask;
	createInfo.dependencyCount = 1U;
	createInfo.pDependencies = &dependency;
	return device.createRenderPassUnique(createInfo);
}
} // namespace

Renderer Renderer::make(vk::Device device, vk::Format colour, vk::Format depth) {
	if (!device) { return {}; }
	return {device, makeRenderPass(device, colour, depth)};
}

vk::UniqueFramebuffer Renderer::makeFramebuffer(RenderImage const& image) const {
	if (!image.colour.view) { return {}; }
	auto const attachments = std::array{image.colour.view, image.depth.view};
	auto const extent = image.colour.extent;
	auto fci = vk::FramebufferCreateInfo({}, *renderPass, static_cast<std::uint32_t>(attachments.size()), attachments.data(), extent.width, extent.height, 1U);
	return device.createFramebufferUnique(fci);
}

void Renderer::render(RenderTarget const& renderTarget, Rgba clear, vk::CommandBuffer primary, std::span<vk::CommandBuffer const> recorded) const {
	if (!renderTarget.framebuffer) { return; }

	auto barrier = ImageBarrier{};
	barrier.access = {{}, vk::AccessFlagBits::eColorAttachmentWrite};
	barrier.stages = {vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput};
	barrier.layouts = {vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal};
	barrier(primary, renderTarget.colour.image);
	barrier.access = {{}, vk::AccessFlagBits::eDepthStencilAttachmentWrite};
	barrier.stages.first = barrier.stages.second = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;
	barrier.layouts.second = vk::ImageLayout::eDepthStencilAttachmentOptimal;
	barrier.aspects = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	barrier(primary, renderTarget.depth.image);
	auto const extent = renderTarget.colour.extent;
	auto const renderArea = vk::Rect2D({}, extent);
	auto const c = clear.normalize();
	vk::ClearValue const cvs[] = {vk::ClearColorValue(std::array{c.x, c.y, c.z, c.w}), vk::ClearDepthStencilValue(1.0f)};
	auto const cvsize = static_cast<std::uint32_t>(std::size(cvs));
	primary.beginRenderPass({*renderPass, renderTarget.framebuffer, renderArea, cvsize, cvs}, vk::SubpassContents::eSecondaryCommandBuffers);
	primary.executeCommands(static_cast<std::uint32_t>(recorded.size()), recorded.data());
	primary.endRenderPass();
}

void Renderer::blit(vk::CommandBuffer primary, RenderTarget const& renderTarget, vk::Image dst, vk::ImageLayout final) const {
	if (!renderTarget.framebuffer) { return; }

	auto isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	auto const extent = renderTarget.colour.extent;
	auto const offsets = std::array{vk::Offset3D(), vk::Offset3D(static_cast<int>(extent.width), static_cast<int>(extent.height), 1)};
	auto blit = vk::ImageBlit(isrl, offsets, isrl, offsets);
	auto barrier = ImageBarrier{};
	barrier.access = {vk::AccessFlagBits::eColorAttachmentWrite, vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite};
	barrier.stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer};
	barrier.layouts = {vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal};
	barrier.aspects = vk::ImageAspectFlagBits::eColor;
	barrier(primary, dst);
	barrier.layouts = {vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal};
	barrier(primary, renderTarget.colour.image);
	barrier.layouts.first = vk::ImageLayout::eTransferDstOptimal;
	primary.blitImage(renderTarget.colour.image, barrier.layouts.second, dst, barrier.layouts.first, blit, vk::Filter::eLinear);

	if (final != vk::ImageLayout::eUndefined) {
		barrier.access = {vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite, {}};
		barrier.stages = {vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe};
		barrier.layouts = {vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR};
		barrier(primary, dst);
	}
}
} // namespace vf
