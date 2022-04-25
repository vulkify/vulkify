#pragma once
#include <detail/vram.hpp>

namespace vf {
struct ImageCache {
	vk::ImageCreateInfo& setDepth() {
		static constexpr auto flags = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment;
		info = vk::ImageCreateInfo();
		info.usage = flags;
		aspect |= vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		return info;
	}

	vk::ImageCreateInfo& setColour() {
		static constexpr auto flags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
		info = vk::ImageCreateInfo();
		info.usage = flags;
		aspect |= vk::ImageAspectFlagBits::eColor;
		return info;
	}

	bool ready(vk::Extent3D extent, vk::Format format) const noexcept { return image && extent == info.extent && info.format == format; }

	VKImage make(vk::Extent3D extent, vk::Format format) {
		info.extent = extent;
		info.format = format;
		vram.device.defer(std::move(image), std::move(view));
		image = vram.makeImage(info, usage, name.c_str());
		view = vram.device.makeImageView(image->resource, format, aspect);
		return peek();
	}

	VKImage refresh(vk::Extent3D extent, vk::Format format) {
		if (!ready(extent, format)) { make(extent, format); }
		return peek();
	}

	VKImage peek() const noexcept { return {image->resource, view ? *view : vk::ImageView(), {info.extent.width, info.extent.height}}; }

	Vram vram{};
	std::string name{};
	vk::ImageCreateInfo info{};
	UniqueImage image{};
	vk::UniqueImageView view{};
	vk::ImageAspectFlags aspect{};
	VmaMemoryUsage usage = VMA_MEMORY_USAGE_GPU_ONLY;
	vk::ImageViewType viewType = vk::ImageViewType::e2D;
};
} // namespace vf
