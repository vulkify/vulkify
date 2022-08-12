#include <detail/trace.hpp>
#include <detail/vulkan_swapchain.hpp>
#include <algorithm>
#include <cassert>
#include <limits>
#include <span>

namespace vf::refactor {
namespace {
constexpr vk::Format srgb_formats_v[] = {vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Srgb};
constexpr vk::Format linear_formats_v[] = {vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8A8Unorm};

constexpr bool is_linear(vk::Format format) {
	return std::find(std::begin(linear_formats_v), std::end(linear_formats_v), format) != std::end(linear_formats_v);
}

template <std::size_t N>
vk::Format image_format(std::span<vk::SurfaceFormatKHR const> available, std::span<vk::Format const, N> targets) noexcept {
	assert(!available.empty());
	vk::Format ranked[N]{};
	for (auto const format : available) {
		if (format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
			for (std::size_t i = 0; i < targets.size(); ++i) {
				if (format == targets[i]) { ranked[i] = format.format; }
			}
		}
	}
	for (auto const format : ranked) {
		if (format != vk::Format()) { return format; }
	}
	return available.front().format;
}

constexpr std::uint32_t image_count(vk::SurfaceCapabilitiesKHR const& caps) noexcept {
	if (caps.maxImageCount < caps.minImageCount) { return std::max(3U, caps.minImageCount); }
	return std::clamp(3U, caps.minImageCount, caps.maxImageCount);
}

constexpr vk::Extent2D image_extent(vk::SurfaceCapabilitiesKHR const& caps, glm::ivec2 const fb) noexcept {
	constexpr auto limitless_v = std::numeric_limits<std::uint32_t>::max();
	if (caps.currentExtent.width < limitless_v && caps.currentExtent.height < limitless_v) { return caps.currentExtent; }
	auto const x = std::clamp(static_cast<std::uint32_t>(fb.x), caps.minImageExtent.width, caps.maxImageExtent.width);
	auto const y = std::clamp(static_cast<std::uint32_t>(fb.y), caps.minImageExtent.height, caps.maxImageExtent.height);
	return vk::Extent2D{x, y};
}

constexpr bool valid(glm::ivec2 extent) { return extent.x > 0 && extent.y > 0; }

vk::SwapchainCreateInfoKHR make_swci(VulkanDevice const& device, std::span<vk::SurfaceFormatKHR const> formats, vk::SurfaceKHR surface, glm::uvec2 extent,
									 bool linear) {
	vk::SwapchainCreateInfoKHR ret;
	ret.surface = surface;
	ret.presentMode = vk::PresentModeKHR::eFifo;
	ret.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
	ret.queueFamilyIndexCount = 1U;
	ret.pQueueFamilyIndices = &device.queue.family;
	ret.imageColorSpace = vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
	ret.imageArrayLayers = 1U;
	std::span const targets = linear ? linear_formats_v : srgb_formats_v;
	ret.imageFormat = image_format(formats, targets);
	auto const caps = device.gpu.getSurfaceCapabilitiesKHR(surface);
	ret.imageExtent = image_extent(caps, extent);
	ret.minImageCount = image_count(caps);
	return ret;
}
} // namespace

VulkanSwapchain VulkanSwapchain::make(VulkanDevice const& device, std::span<vk::SurfaceFormatKHR const> formats, vk::SurfaceKHR surface,
									  vk::PresentModeKHR mode, glm::ivec2 extent, bool linear) {
	if (!device) { return {}; }
	auto ret = VulkanSwapchain{device, surface, formats, linear};
	if (ret.refresh(extent, mode) != vk::Result::eSuccess) { return {}; }
	ret.linear = is_linear(ret.info.imageFormat);
	return ret;
}

vk::SwapchainCreateInfoKHR VulkanSwapchain::make_info(glm::uvec2 const extent) const { return make_swci(device, formats, surface, extent, linear); }

vk::Result VulkanSwapchain::refresh(glm::uvec2 const extent, vk::PresentModeKHR const mode) {
	if (!device) { return vk::Result::eErrorDeviceLost; }
	info = make_info(extent);
	info.oldSwapchain = *swapchain.swapchain;
	info.presentMode = mode;
	vk::SwapchainKHR vks;
	auto const ret = device.device.createSwapchainKHR(&info, nullptr, &vks);
	if (ret != vk::Result::eSuccess) { return ret; }
	device.defer(std::move(swapchain));
	swapchain = {};
	swapchain.swapchain = vk::UniqueSwapchainKHR(vks, device.device);
	auto const images = device.device.getSwapchainImagesKHR(*swapchain.swapchain);
	for (std::size_t i = 0; i < images.size(); ++i) {
		swapchain.views.push_back(device.make_image_view(images[i], info.imageFormat, vk::ImageAspectFlagBits::eColor));
		swapchain.images.push_back({images[i], *swapchain.views[i], info.imageExtent});
	}
	return ret;
}

VulkanSwapchain::Acquire VulkanSwapchain::acquire(vk::Semaphore const signal, glm::uvec2 const extent) {
	if (!valid(extent)) { return {}; }
	if (!device) { return {}; }
	static constexpr auto max_wait_v = std::numeric_limits<std::uint64_t>::max();
	std::uint32_t idx{};
	auto result = device.device.acquireNextImageKHR(*swapchain.swapchain, max_wait_v, signal, {}, &idx);
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
		refresh(extent, info.presentMode);
		return {};
	}
	auto const i = std::size_t(idx);
	assert(i < swapchain.images.size());
	return {swapchain.images[i], idx};
}

void VulkanSwapchain::submit(vk::CommandBuffer const cb, SwapchainSync const& sync) {
	if (!device) { return; }
	static constexpr vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	vk::SubmitInfo submitInfo;
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.commandBufferCount = 1U;
	submitInfo.pCommandBuffers = &cb;
	submitInfo.waitSemaphoreCount = 1U;
	submitInfo.pWaitSemaphores = &sync.draw;
	submitInfo.signalSemaphoreCount = 1U;
	submitInfo.pSignalSemaphores = &sync.present;
	auto lock = std::scoped_lock(*device.queue_mutex);
	auto res = device.queue.queue.submit(1U, &submitInfo, sync.drawn);
	if (res != vk::Result::eSuccess) { VF_TRACE("vf::(internal)", trace::Type::eError, "Queue submit failure!"); }
}

void VulkanSwapchain::present(Acquire const& acquired, vk::Semaphore const wait, glm::uvec2 const extent) {
	if (!device || !acquired) { return; }
	vk::PresentInfoKHR info;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &wait;
	info.swapchainCount = 1;
	info.pSwapchains = &*swapchain.swapchain;
	info.pImageIndices = &*acquired.index;
	auto res = device.queue.queue.presentKHR(&info);
	if (res != vk::Result::eSuccess && res != vk::Result::eSuboptimalKHR) { refresh(extent, this->info.presentMode); }
}
} // namespace vf::refactor
