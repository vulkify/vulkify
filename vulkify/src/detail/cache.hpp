#pragma once
#include <detail/rotator.hpp>
#include <detail/trace.hpp>
#include <detail/vram.hpp>

namespace vf {
struct ImageCache {
	using Extent = glm::uvec2;

	struct Info {
		Vram vram{};
		std::string name{};
		vk::ImageCreateInfo info{};
		vk::ImageAspectFlags aspect{};
		bool preferHost{false};
	};

	Info info{};

	UniqueImage image{};
	vk::UniqueImageView view{};

	static constexpr Extent scale2D(Extent extent, float value) { return glm::vec2(extent) * value; }

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

	vk::ImageCreateInfo& setTexture(bool const transferSrc) {
		static constexpr auto flags = vk::ImageUsageFlagBits::eSampled;
		info.info = vk::ImageCreateInfo();
		info.info.usage = flags;
		info.info.format = info.vram.textureFormat;
		info.info.usage |= vk::ImageUsageFlagBits::eTransferDst;
		if (transferSrc) { info.info.usage |= vk::ImageUsageFlagBits::eTransferSrc; }
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
		info.vram.device.defer(std::move(image), std::move(view));
		image = info.vram.makeImage(info.info, info.preferHost, info.name.c_str());
		if (!image) { return false; }
		view = info.vram.device.makeImageView(image->resource, format, info.aspect);
		return *view;
	}

	VKImage refresh(Extent const extent, vk::Format format = {}) {
		if (format == vk::Format()) { format = info.info.format; }
		if (!ready(extent, format)) { make(extent, format); }
		return peek();
	}

	VKImage peek() const noexcept { return {image->resource, view ? *view : vk::ImageView(), {info.info.extent.width, info.info.extent.height}}; }
};

struct BufferCache {
	std::string name{};
	mutable vk::BufferCreateInfo info{};
	mutable Rotator<UniqueBuffer, 4> buffers{};
	std::vector<std::byte> data{std::byte{}};
	Vram const* vram{};

	BufferCache() = default;

	BufferCache(Vram const& vram, vk::BufferUsageFlagBits usage) : vram(&vram) {
		info.usage = usage;
		info.size = 1;
		for (std::size_t i = 0; i < vram.buffering; ++i) { buffers.push(vram.makeBuffer(info, true, this->name.c_str())); }
	}

	explicit operator bool() const { return vram && !buffers.storage.empty(); }

	VmaBuffer const& get(bool next) const {
		static auto const blank_v = VmaBuffer{};
		if (!vram) { return blank_v; }
		auto& buffer = buffers.get();
		if (buffer->size < data.size()) {
			info.size = data.size();
			vram->device.defer(std::move(buffer));
			buffer = vram->makeBuffer(info, true, name.c_str());
		}
		buffer->write(data.data(), data.size());
		if (next) { buffers.next(); }
		return buffer;
	}
};
} // namespace vf
