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
	return {makeRenderPass(device, format, samples), device};
}

vk::UniqueFramebuffer Renderer::makeFramebuffer(RenderTarget const& target) const {
	if (!target.colour.view) { return {}; }
	auto attachments = ktl::fixed_vector<vk::ImageView, 2>{target.colour.view};
	if (target.resolve.view) { attachments.push_back(target.resolve.view); }
	auto const extent = target.extent;
	auto fci = vk::FramebufferCreateInfo({}, *renderPass, static_cast<std::uint32_t>(attachments.size()), attachments.data(), extent.width, extent.height, 1U);
	return device.createFramebufferUnique(fci);
}

void Renderer::Frame::render(Rgba clear, std::span<vk::CommandBuffer const> recorded) const {
	if (!framebuffer) { return; }
	auto const extent = framebuffer.extent;
	auto const renderArea = vk::Rect2D({}, extent);
	auto const c = clear.normalize();
	vk::ClearValue const cvs[] = {vk::ClearColorValue(std::array{c.x, c.y, c.z, c.w})};
	auto const cvsize = static_cast<std::uint32_t>(std::size(cvs));
	cmd.beginRenderPass({*renderer.renderPass, framebuffer, renderArea, cvsize, cvs}, vk::SubpassContents::eSecondaryCommandBuffers);
	cmd.executeCommands(static_cast<std::uint32_t>(recorded.size()), recorded.data());
	cmd.endRenderPass();
}

void Renderer::Frame::blit(VKImage const& src, VKImage const& dst) const {
	auto const srcExtent = glm::uvec2(src.extent.width, src.extent.height);
	auto const dstExtent = glm::uvec2(dst.extent.width, dst.extent.height);
	Vram::blit(cmd, src.image, dst.image, {srcExtent}, {dstExtent}, vk::Filter::eLinear);
}

void Renderer::Frame::undefToColour(std::span<VKImage const> images) const {
	auto barrier = ImageBarrier{};
	barrier.access = {{}, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead};
	barrier.stages = {vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput};
	for (auto const image : images) { barrier(cmd, image.image, vk::ImageLayout::eColorAttachmentOptimal); }
}

void Renderer::Frame::colourToPresent(VKImage const& image) const {
	auto barrier = ImageBarrier{};
	barrier.access = {vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, {}};
	barrier.stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe};
	barrier(cmd, image.image, {vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR});
}

void Renderer::Frame::colourToTfr(VKImage const& src, VKImage const& dst) const {
	auto barrier = ImageBarrier{};
	barrier.access = {vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
					  vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite};
	barrier.stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer};
	barrier(cmd, src.image, {vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal});
	barrier(cmd, dst.image, {vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal});
}

void Renderer::Frame::tfrToPresent(VKImage const& image) const {
	auto barrier = ImageBarrier{};
	barrier.access = {vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite, {}};
	barrier.stages = {vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe};
	barrier(cmd, image.image, {vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR});
}
} // namespace vf
