#include <detail/renderer.hpp>
#include <detail/vram.hpp>

namespace vf {
namespace {
vk::UniqueRenderPass makeRenderPass(vk::Device device, vk::Format format, vk::SampleCountFlagBits samples) {
	auto attachments = ktl::fixed_vector<vk::AttachmentDescription, 2>{};
	auto colour = vk::AttachmentReference{};
	auto resolve = vk::AttachmentReference{};

	auto subpass = vk::SubpassDescription{};
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colour;

	colour.attachment = static_cast<std::uint32_t>(attachments.size());
	colour.layout = vk::ImageLayout::eColorAttachmentOptimal;
	auto attachment = vk::AttachmentDescription{};
	attachment.format = format;
	attachment.samples = samples;
	attachment.loadOp = vk::AttachmentLoadOp::eClear;
	attachment.storeOp = samples > vk::SampleCountFlagBits::e1 ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
	attachment.initialLayout = attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
	attachments.push_back(attachment);

	if (samples > vk::SampleCountFlagBits::e1) {
		subpass.pResolveAttachments = &resolve;
		resolve.attachment = static_cast<std::uint32_t>(attachments.size());
		resolve.layout = vk::ImageLayout::eColorAttachmentOptimal;

		attachment.samples = vk::SampleCountFlagBits::e1;
		attachment.loadOp = vk::AttachmentLoadOp::eDontCare;
		attachment.storeOp = vk::AttachmentStoreOp::eStore;
		attachments.push_back(attachment);
	}

	vk::RenderPassCreateInfo createInfo;
	createInfo.attachmentCount = static_cast<std::uint32_t>(attachments.size());
	createInfo.pAttachments = attachments.data();
	createInfo.subpassCount = 1U;
	createInfo.pSubpasses = &subpass;
	vk::SubpassDependency dependency;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
	dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead;
	dependency.srcStageMask = dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	createInfo.dependencyCount = 1U;
	createInfo.pDependencies = &dependency;
	return device.createRenderPassUnique(createInfo);
}
} // namespace

Renderer Renderer::make(vk::Device device, vk::Format format, vk::SampleCountFlagBits samples) {
	if (!device) { return {}; }
	return {device, makeRenderPass(device, format, samples)};
}

vk::UniqueFramebuffer Renderer::makeFramebuffer(RenderImage const& image) const {
	if (!image.colour.view) { return {}; }
	auto attachments = ktl::fixed_vector<vk::ImageView, 2>{image.colour.view};
	if (image.resolve.view) { attachments.push_back(image.resolve.view); }
	auto const extent = image.colour.extent;
	auto fci = vk::FramebufferCreateInfo({}, *renderPass, static_cast<std::uint32_t>(attachments.size()), attachments.data(), extent.width, extent.height, 1U);
	return device.createFramebufferUnique(fci);
}

void Renderer::toColourOptimal(vk::CommandBuffer primary, std::span<vk::Image const> images) const {
	auto barrier = ImageBarrier{};
	barrier.access = {{}, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead};
	barrier.stages = {vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput};
	barrier.layouts = {vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal};
	for (auto const image : images) { barrier(primary, image); }
}

void Renderer::render(RenderTarget const& renderTarget, Rgba clear, vk::CommandBuffer primary, std::span<vk::CommandBuffer const> recorded) const {
	if (!renderTarget.framebuffer) { return; }

	auto const extent = renderTarget.colour.extent;
	auto const renderArea = vk::Rect2D({}, extent);
	auto const c = clear.normalize();
	vk::ClearValue const cvs[] = {vk::ClearColorValue(std::array{c.x, c.y, c.z, c.w})};
	auto const cvsize = static_cast<std::uint32_t>(std::size(cvs));
	primary.beginRenderPass({*renderPass, renderTarget.framebuffer, renderArea, cvsize, cvs}, vk::SubpassContents::eSecondaryCommandBuffers);
	primary.executeCommands(static_cast<std::uint32_t>(recorded.size()), recorded.data());
	primary.endRenderPass();
}

void Renderer::toPresentSrc(vk::CommandBuffer primary, vk::Image present) const {
	auto barrier = ImageBarrier{};
	barrier.access = {vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, {}};
	barrier.stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe};
	barrier.layouts = {vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR};
	barrier(primary, present);
}
} // namespace vf
