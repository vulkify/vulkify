#include <detail/gfx_command_buffer.hpp>
#include <detail/renderer.hpp>
#include <ktl/fixed_vector.hpp>

namespace vf {
namespace {
vk::UniqueRenderPass make_render_pass(vk::Device device, vk::Format colour, bool depth, vk::SampleCountFlagBits samples) {
	auto attachments = ktl::fixed_vector<vk::AttachmentDescription, 3>{};
	auto colour_ref = vk::AttachmentReference{};
	auto depth_ref = vk::AttachmentReference{};
	auto resolve_ref = vk::AttachmentReference{};

	auto subpass = vk::SubpassDescription{};
	subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colour_ref;

	colour_ref.attachment = static_cast<std::uint32_t>(attachments.size());
	colour_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
	auto attachment = vk::AttachmentDescription{};
	attachment.format = colour;
	attachment.samples = samples;
	attachment.loadOp = vk::AttachmentLoadOp::eClear;
	attachment.storeOp = samples > vk::SampleCountFlagBits::e1 ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore;
	attachment.initialLayout = attachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
	attachments.push_back(attachment);

	if (depth) {
		subpass.pDepthStencilAttachment = &depth_ref;
		depth_ref.attachment = static_cast<std::uint32_t>(attachments.size());
		depth_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		auto attachment = vk::AttachmentDescription{};
		attachment.format = vk::Format::eD16Unorm;
		attachment.samples = samples;
		attachment.loadOp = vk::AttachmentLoadOp::eClear;
		attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
		attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		attachment.initialLayout = attachment.finalLayout = depth_ref.layout;
		attachments.push_back(attachment);
	}

	if (samples > vk::SampleCountFlagBits::e1) {
		subpass.pResolveAttachments = &resolve_ref;
		resolve_ref.attachment = static_cast<std::uint32_t>(attachments.size());
		resolve_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;

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
	return {make_render_pass(device, format, true, samples), device};
}

vk::UniqueFramebuffer Renderer::make_framebuffer(RenderTarget const& target) const {
	if (!target.colour.view) { return {}; }
	auto attachments = ktl::fixed_vector<vk::ImageView, 3>{target.colour.view};
	if (target.depth.view) { attachments.push_back(target.depth.view); }
	if (target.resolve.view) { attachments.push_back(target.resolve.view); }
	auto const extent = target.extent;
	auto fci = vk::FramebufferCreateInfo({}, *render_pass, static_cast<std::uint32_t>(attachments.size()), attachments.data(), extent.width, extent.height, 1U);
	return device.createFramebufferUnique(fci);
}

void Renderer::Frame::render(Rgba clear, std::span<vk::CommandBuffer const> recorded) const {
	if (!framebuffer) { return; }
	auto const extent = framebuffer.extent;
	auto const renderArea = vk::Rect2D({}, extent);
	auto const c = clear.normalize();
	vk::ClearValue const cvs[] = {vk::ClearColorValue(std::array{c.x, c.y, c.z, c.w}), vk::ClearDepthStencilValue(1.0f)};
	auto const cvsize = static_cast<std::uint32_t>(std::size(cvs));
	cmd.beginRenderPass({*renderer.render_pass, framebuffer, renderArea, cvsize, cvs}, vk::SubpassContents::eSecondaryCommandBuffers);
	cmd.executeCommands(static_cast<std::uint32_t>(recorded.size()), recorded.data());
	cmd.endRenderPass();
}

void Renderer::Frame::blit(ImageView const& src, ImageView const& dst) const {
	auto const srcExtent = glm::uvec2(src.extent.width, src.extent.height);
	auto const dstExtent = glm::uvec2(dst.extent.width, dst.extent.height);
	ImageWriter::blit(cmd, src.image, dst.image, {srcExtent}, {dstExtent}, vk::Filter::eLinear);
}

void Renderer::Frame::undef_to_depth(ImageView const& src) const {
	auto barrier = ImageBarrier{};
	barrier.access = {{}, vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead};
	barrier.stages = {vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests};
	barrier.aspects = vk::ImageAspectFlagBits::eDepth;
	barrier(cmd, src.image, vk::ImageLayout::eDepthStencilAttachmentOptimal);
}

void Renderer::Frame::undef_to_colour(std::span<ImageView const> images) const {
	auto barrier = ImageBarrier{};
	barrier.access = {{}, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eColorAttachmentRead};
	barrier.stages = {vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput};
	for (auto const image : images) { barrier(cmd, image.image, vk::ImageLayout::eColorAttachmentOptimal); }
}

void Renderer::Frame::colour_to_present(ImageView const& image) const {
	auto barrier = ImageBarrier{};
	barrier.access = {vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite, {}};
	barrier.stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe};
	barrier(cmd, image.image, {vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR});
}

void Renderer::Frame::colour_to_tfr(ImageView const& src, ImageView const& dst) const {
	auto barrier = ImageBarrier{};
	barrier.access = {vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
					  vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite};
	barrier.stages = {vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eTransfer};
	barrier(cmd, src.image, {vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::eTransferSrcOptimal});
	barrier(cmd, dst.image, {vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal});
}

void Renderer::Frame::tfr_to_present(ImageView const& image) const {
	auto barrier = ImageBarrier{};
	barrier.access = {vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite, {}};
	barrier.stages = {vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe};
	barrier(cmd, image.image, {vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR});
}
} // namespace vf
