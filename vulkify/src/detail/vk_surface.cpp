// #include <detail/defer_queue.hpp>
// #include <detail/expect.hpp>
#include <detail/trace.hpp>
#include <detail/vk_instance.hpp>
#include <detail/vk_surface.hpp>
#include <algorithm>
#include <cassert>
#include <limits>
#include <span>

namespace vf {
namespace {
constexpr vk::Format imageFormat(std::span<vk::SurfaceFormatKHR const> formats) noexcept {
	constexpr vk::Format targets[] = {vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8A8Unorm};
	for (auto const format : formats) {
		if (format.colorSpace == vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
			for (auto const target : targets) {
				if (format == target) { return format.format; }
			}
		}
	}
	return formats.empty() ? vk::Format() : formats.front().format;
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

vk::UniqueImageView makeImageView(vk::Device const device, vk::Image const image, vk::Format const format) {
	vk::ImageViewCreateInfo info;
	info.viewType = vk::ImageViewType::e2D;
	info.format = format;
	info.components.r = vk::ComponentSwizzle::eR;
	info.components.g = vk::ComponentSwizzle::eG;
	info.components.b = vk::ComponentSwizzle::eB;
	info.components.a = vk::ComponentSwizzle::eA;
	info.subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
	info.image = image;
	return device.createImageViewUnique(info);
}
} // namespace

vk::SwapchainCreateInfoKHR VKSurface::makeInfo(Device const& device, vk::SurfaceKHR const surface, glm::ivec2 const framebuffer) {
	vk::SwapchainCreateInfoKHR ret;
	ret.surface = surface;
	ret.presentMode = vk::PresentModeKHR::eFifo;
	ret.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
	ret.queueFamilyIndexCount = 1U;
	ret.pQueueFamilyIndices = &device.queue.family;
	ret.imageColorSpace = vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear;
	ret.imageArrayLayers = 1U;
	ret.imageFormat = imageFormat(device.gpu.formats);
	auto const caps = device.gpu.device.getSurfaceCapabilitiesKHR(surface);
	ret.imageExtent = imageExtent(caps, framebuffer);
	ret.minImageCount = imageCount(caps);
	return ret;
}

vk::Result VKSurface::refresh(Device const& device, glm::ivec2 const framebuffer) {
	if (framebuffer.x <= 0 || framebuffer.y <= 0) { return vk::Result::eNotReady; }
	info = makeInfo(device, surface, framebuffer);
	info.oldSwapchain = *swapchain.swapchain;
	vk::SwapchainKHR vks;
	auto const ret = device.device.createSwapchainKHR(&info, nullptr, &vks);
	if (ret != vk::Result::eSuccess) { return ret; }
	VF_TRACEF("Swapchain resized: {}x{}", info.imageExtent.width, info.imageExtent.height);
	if (deferQueue) {
		// deferQueue->defer(std::move(swapchain)); // defer destruction of current swapchain and its image views if possible
	} else {
		device.device.waitIdle(); // otherwise stall device
	}
	swapchain = {};
	swapchain.swapchain = vk::UniqueSwapchainKHR(vks, device.device);
	auto const images = device.device.getSwapchainImagesKHR(*swapchain.swapchain);
	for (std::size_t i = 0; i < images.size(); ++i) {
		swapchain.views.push_back(makeImageView(device.device, images[i], info.imageFormat));
		swapchain.images.push_back({images[i], *swapchain.views[i], info.imageExtent});
	}
	return ret;
}

std::optional<VKSurface::Acquire> VKSurface::acquire(Device const& device, vk::Semaphore const signal, glm::ivec2 const framebuffer) {
	static constexpr auto max_wait_v = std::numeric_limits<std::uint64_t>::max();
	std::uint32_t idx{};
	auto result = presentResult(device.device.acquireNextImageKHR(*swapchain.swapchain, max_wait_v, signal, {}, &idx));
	if (!result) { return std::nullopt; }
	if (*result == PresentOutcome::eNotReady) {
		refresh(device, framebuffer);
		return std::nullopt;
	}
	auto const i = std::size_t(idx);
	assert(i < swapchain.images.size());
	return Acquire{swapchain.images[i], idx};
}

vk::Result VKSurface::submit(Device const& device, vk::CommandBuffer const cb, Sync const& sync) {
	static constexpr vk::PipelineStageFlags waitStages = vk::PipelineStageFlagBits::eTopOfPipe;
	vk::SubmitInfo submitInfo;
	submitInfo.pWaitDstStageMask = &waitStages;
	submitInfo.commandBufferCount = 1U;
	submitInfo.pCommandBuffers = &cb;
	submitInfo.waitSemaphoreCount = 1U;
	submitInfo.pWaitSemaphores = &sync.wait;
	submitInfo.signalSemaphoreCount = 1U;
	submitInfo.pSignalSemaphores = &sync.ssignal;
	auto const ret = device.queue.queue.submit(1U, &submitInfo, sync.fsignal);
	return ret;
}

PresentResult VKSurface::present(Device const& device, Acquire const& acquired, vk::Semaphore const wait, glm::ivec2 const framebuffer) {
	vk::PresentInfoKHR info;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &wait;
	info.swapchainCount = 1;
	info.pSwapchains = &*swapchain.swapchain;
	info.pImageIndices = &acquired.index;
	auto ret = presentResult(device.queue.queue.presentKHR(&info));
	if (ret && *ret == PresentOutcome::eNotReady) { refresh(device, framebuffer); }
	return ret;
}
} // namespace vf
