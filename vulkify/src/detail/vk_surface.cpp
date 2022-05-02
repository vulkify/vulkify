#include <detail/trace.hpp>
#include <detail/vk_surface.hpp>
#include <algorithm>
#include <cassert>
#include <limits>
#include <span>

namespace vf {
namespace {
constexpr vk::Format srgb_formats_v[] = {vk::Format::eR8G8B8A8Srgb, vk::Format::eB8G8R8A8Srgb};
constexpr vk::Format linear_formats_v[] = {vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8A8Unorm};

constexpr bool isLinear(vk::Format format) { return std::find(std::begin(linear_formats_v), std::end(linear_formats_v), format) != std::end(linear_formats_v); }

template <std::size_t N>
vk::Format imageFormat(std::span<vk::SurfaceFormatKHR const> available, std::span<vk::Format const, N> targets) noexcept {
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

constexpr std::uint32_t imageCount(vk::SurfaceCapabilitiesKHR const& caps) noexcept {
	if (caps.maxImageCount < caps.minImageCount) { return std::max(3U, caps.minImageCount); }
	return std::clamp(3U, caps.minImageCount, caps.maxImageCount);
}

constexpr vk::Extent2D imageExtent(vk::SurfaceCapabilitiesKHR const& caps, glm::ivec2 const fb) noexcept {
	constexpr auto limitless_v = std::numeric_limits<std::uint32_t>::max();
	if (caps.currentExtent.width < limitless_v && caps.currentExtent.height < limitless_v) { return caps.currentExtent; }
	auto const x = std::clamp(static_cast<std::uint32_t>(fb.x), caps.minImageExtent.width, caps.maxImageExtent.width);
	auto const y = std::clamp(static_cast<std::uint32_t>(fb.y), caps.minImageExtent.height, caps.maxImageExtent.height);
	return vk::Extent2D{x, y};
}

constexpr PresentResult presentResult(vk::Result const result) noexcept {
	switch (result) {
	case vk::Result::eSuccess: return PresentOutcome::eSuccess;
	case vk::Result::eSuboptimalKHR:
	case vk::Result::eErrorOutOfDateKHR: return PresentOutcome::eNotReady;
	default: break;
	}
	return result;
}

constexpr bool valid(glm::ivec2 extent) { return extent.x > 0 && extent.y > 0; }

vk::SwapchainCreateInfoKHR makeSwci(VKDevice const& device, VKGpu const& gpu, vk::SurfaceKHR surface, glm::ivec2 framebuffer, bool linear) {
	vk::SwapchainCreateInfoKHR ret;
	ret.surface = surface;
	ret.presentMode = vk::PresentModeKHR::eFifo;
	ret.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst;
	ret.queueFamilyIndexCount = 1U;
	ret.pQueueFamilyIndices = &device.queue.family;
	ret.imageColorSpace = vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
	ret.imageArrayLayers = 1U;
	std::span const targets = linear ? linear_formats_v : srgb_formats_v;
	ret.imageFormat = imageFormat(gpu.formats, targets);
	auto const caps = gpu.device.getSurfaceCapabilitiesKHR(surface);
	ret.imageExtent = imageExtent(caps, framebuffer);
	ret.minImageCount = imageCount(caps);
	return ret;
}
} // namespace

VKSurface VKSurface::make(VKDevice const& device, VKGpu const& gpu, vk::SurfaceKHR surface, glm::ivec2 framebuffer, bool linear) {
	if (!device) { return {}; }
	auto ret = VKSurface{device, gpu, surface, linear};
	if (ret.refresh(framebuffer) != vk::Result::eSuccess) { return {}; }
	ret.linear = isLinear(ret.info.imageFormat);
	return ret;
}

vk::SwapchainCreateInfoKHR VKSurface::makeInfo(glm::ivec2 const framebuffer) const { return makeSwci(device, gpu, surface, framebuffer, linear); }

vk::Result VKSurface::refresh(glm::ivec2 const framebuffer) {
	if (!device) { return vk::Result::eErrorDeviceLost; }
	if (!valid(framebuffer)) { return vk::Result::eNotReady; }
	info = makeInfo(framebuffer);
	info.oldSwapchain = *swapchain.swapchain;
	vk::SwapchainKHR vks;
	auto const ret = device.device.createSwapchainKHR(&info, nullptr, &vks);
	if (ret != vk::Result::eSuccess) { return ret; }
	VF_TRACEF("Swapchain resized: {}x{}", info.imageExtent.width, info.imageExtent.height);
	device.defer(std::move(swapchain.swapchain));
	for (auto& view : swapchain.views) { device.defer(std::move(view)); }
	swapchain = {};
	swapchain.swapchain = vk::UniqueSwapchainKHR(vks, device.device);
	auto const images = device.device.getSwapchainImagesKHR(*swapchain.swapchain);
	for (std::size_t i = 0; i < images.size(); ++i) {
		swapchain.views.push_back(device.makeImageView(images[i], info.imageFormat, vk::ImageAspectFlagBits::eColor));
		swapchain.images.push_back({images[i], *swapchain.views[i], info.imageExtent});
	}
	return ret;
}

std::optional<VKSurface::Acquire> VKSurface::acquire(vk::Semaphore const signal, glm::ivec2 const framebuffer) {
	if (!valid(framebuffer)) { return {}; }
	if (!device) { return {}; }
	static constexpr auto max_wait_v = std::numeric_limits<std::uint64_t>::max();
	std::uint32_t idx{};
	auto result = presentResult(device.device.acquireNextImageKHR(*swapchain.swapchain, max_wait_v, signal, {}, &idx));
	if (!result) { return std::nullopt; }
	if (*result == PresentOutcome::eNotReady) {
		refresh(framebuffer);
		return std::nullopt;
	}
	auto const i = std::size_t(idx);
	assert(i < swapchain.images.size());
	return Acquire{swapchain.images[i], idx};
}

vk::Result VKSurface::submit(vk::CommandBuffer const cb, VKSync const& sync) {
	if (!device) { return vk::Result::eErrorDeviceLost; }
	static constexpr vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eTopOfPipe;
	vk::SubmitInfo submitInfo;
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.commandBufferCount = 1U;
	submitInfo.pCommandBuffers = &cb;
	submitInfo.waitSemaphoreCount = 1U;
	submitInfo.pWaitSemaphores = &sync.draw;
	submitInfo.signalSemaphoreCount = 1U;
	submitInfo.pSignalSemaphores = &sync.present;
	auto lock = std::scoped_lock(*device.mutex);
	return device.queue.queue.submit(1U, &submitInfo, sync.drawn);
}

PresentResult VKSurface::present(Acquire const& acquired, vk::Semaphore const wait, glm::ivec2 const framebuffer) {
	if (!device) { return vk::Result::eErrorDeviceLost; }
	vk::PresentInfoKHR info;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &wait;
	info.swapchainCount = 1;
	info.pSwapchains = &*swapchain.swapchain;
	info.pImageIndices = &acquired.index;
	auto ret = presentResult(device.queue.queue.presentKHR(&info));
	if (ret && *ret == PresentOutcome::eNotReady) { refresh(framebuffer); }
	return ret;
}
} // namespace vf
