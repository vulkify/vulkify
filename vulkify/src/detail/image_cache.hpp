#pragma once
#include <detail/vram.hpp>

namespace vf {
struct ImageCache {
	struct Info {
		Vram vram{};
		std::string name{};
		vk::ImageCreateInfo info{};
		vk::ImageAspectFlags aspect{};
		bool preferHost{true};
	};

	Info info{};

	UniqueImage image{};
	vk::UniqueImageView view{};

	explicit operator bool() const { return info.vram.device.device; }
	bool operator==(ImageCache const& rhs) const { return !image && !view && !rhs.image && !rhs.view; }

	vk::ImageCreateInfo& setDepth() {
		static constexpr auto flags = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment;
		info.info = vk::ImageCreateInfo();
		info.info.usage = flags;
		info.aspect |= vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		return info.info;
	}

	vk::ImageCreateInfo& setColour() {
		static constexpr auto flags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
		info.info = vk::ImageCreateInfo();
		info.info.usage = flags;
		info.aspect |= vk::ImageAspectFlagBits::eColor;
		return info.info;
	}

	vk::ImageCreateInfo& setTexture(bool transferSrc) {
		static constexpr auto flags = vk::ImageUsageFlagBits::eSampled;
		info.info = vk::ImageCreateInfo();
		info.info.usage = flags;
		info.info.format = info.vram.textureFormat;
		info.info.usage |= vk::ImageUsageFlagBits::eTransferDst;
		if (transferSrc) { info.info.usage |= vk::ImageUsageFlagBits::eTransferSrc; }
		info.aspect |= vk::ImageAspectFlagBits::eColor;
		return info.info;
	}

	bool ready(vk::Extent3D extent, vk::Format format) const noexcept { return image && extent == info.info.extent && info.info.format == format; }

	bool make(vk::Extent3D extent, vk::Format format) {
		info.info.extent = extent;
		info.info.format = format;
		info.vram.device.defer(std::move(image), std::move(view));
		image = info.vram.makeImage(info.info, info.preferHost, info.name.c_str());
		if (!image) { return false; }
		view = info.vram.device.makeImageView(image->resource, format, info.aspect);
		return *view;
	}

	VKImage refresh(vk::Extent3D extent, vk::Format format = {}) {
		if (format == vk::Format()) { format = info.info.format; }
		if (!ready(extent, format)) { make(extent, format); }
		return peek();
	}

	VKImage peek() const noexcept { return {image->resource, view ? *view : vk::ImageView(), {info.info.extent.width, info.info.extent.height}}; }
};
} // namespace vf
