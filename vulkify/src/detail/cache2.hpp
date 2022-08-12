#pragma once
#include <detail/gfx_allocation.hpp>
#include <detail/gfx_device.hpp>
#include <detail/rotator.hpp>
#include <detail/trace.hpp>

namespace vf::refactor {
struct ImageCache {
	using Extent = glm::uvec2;

	struct Info {
		vk::ImageCreateInfo info{};
		vk::ImageAspectFlags aspect{};
		bool prefer_host{false};
	};

	Info info{};

	GfxDevice device{};
	UniqueImage image{};
	vk::UniqueImageView view{};

	static constexpr Extent scale2D(Extent extent, float value) { return glm::vec2(extent) * value; }

	explicit operator bool() const { return device.device.device; }
	bool operator==(ImageCache const& rhs) const { return !image && !view && !rhs.image && !rhs.view; }

	vk::ImageCreateInfo& set_depth() {
		static constexpr auto flags = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment;
		info.info = vk::ImageCreateInfo();
		info.info.usage = flags;
		info.aspect |= vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
		return info.info;
	}

	vk::ImageCreateInfo& set_colour() {
		static constexpr auto flags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
		info.info = vk::ImageCreateInfo();
		info.info.usage = flags;
		info.aspect |= vk::ImageAspectFlagBits::eColor;
		return info.info;
	}

	vk::ImageCreateInfo& set_texture(bool const transfer_src) {
		static constexpr auto flags = vk::ImageUsageFlagBits::eSampled;
		info.info = vk::ImageCreateInfo();
		info.info.usage = flags;
		info.info.format = device.texture_format;
		info.info.usage |= vk::ImageUsageFlagBits::eTransferDst;
		if (transfer_src) { info.info.usage |= vk::ImageUsageFlagBits::eTransferSrc; }
		info.aspect |= vk::ImageAspectFlagBits::eColor;
		return info.info;
	}

	Extent current() const { return {info.info.extent.width, info.info.extent.height}; }

	bool ready(Extent const extent, vk::Format const format) const noexcept {
		if (!image || info.info.format != format) { return false; }
		return current() == extent;
	}

	bool make(Extent const extent, vk::Format const format) {
		info.info.extent = vk::Extent3D(extent.x, extent.y, 1);
		info.info.format = format;
		device.device.defer(std::move(image), std::move(view));
		image = device.make_image(info.info, info.prefer_host);
		if (!image) { return false; }
		view = device.device.make_image_view(image->resource, format, info.aspect);
		return *view;
	}

	ImageView refresh(Extent const extent, vk::Format format = {}) {
		if (format == vk::Format()) { format = info.info.format; }
		if (!ready(extent, format)) { make(extent, format); }
		return peek();
	}

	ImageView peek() const noexcept { return {image->resource, view ? *view : vk::ImageView(), {info.info.extent.width, info.info.extent.height}}; }
};

struct BufferCache {
	GfxDevice device{};
	mutable vk::BufferCreateInfo info{};
	mutable Rotator<UniqueBuffer, 4> buffers{};
	std::vector<std::byte> data{std::byte{}};

	BufferCache() = default;

	BufferCache(GfxDevice const& device, vk::BufferUsageFlagBits usage) : device(device) {
		info.usage = usage;
		info.size = 1;
		for (std::size_t i = 0; i < device.buffering; ++i) { buffers.push(device.make_buffer(info, true)); }
	}

	explicit operator bool() const { return device && !buffers.storage.empty(); }

	VmaBuffer const& get(bool next) const {
		static auto const blank_v = VmaBuffer{};
		if (!device) { return blank_v; }
		auto& buffer = buffers.get();
		if (buffer->size < data.size()) {
			info.size = data.size();
			device.device.defer(std::move(buffer));
			buffer = device.make_buffer(info, true);
		}
		buffer->write(data.data(), data.size());
		if (next) { buffers.next(); }
		return buffer;
	}
};

struct VulkanImage {
	ImageCache cache{};
	vk::UniqueSampler sampler{};

	static VulkanImage make(GfxDevice const& device) { return VulkanImage{.cache = ImageCache{.device = device}}; }

	bool operator==(VulkanImage const& rhs) const { return !sampler && !rhs.sampler && !cache.image && !rhs.cache.image; }
};

template <std::size_t Count>
class GfxBuffer : public GfxAllocation {
  public:
	GfxBuffer(GfxDevice const& device) : GfxAllocation(device, Type::eBuffer) {}

	BufferCache buffers[Count]{};
};

using GfxGeometryBuffer = GfxBuffer<2>;

class GfxImage : public GfxAllocation {
  public:
	GfxImage(GfxDevice const& device) : GfxAllocation(device, Type::eImage) {}

	void replace(ImageCache&& cache) {
		device().device.defer(std::move(image.cache));
		image.cache = std::move(cache);
	}

	VulkanImage image{};
};
} // namespace vf::refactor
