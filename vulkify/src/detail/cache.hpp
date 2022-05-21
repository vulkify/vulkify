#pragma once
#include <detail/rotator.hpp>
#include <detail/trace.hpp>
#include <detail/vram.hpp>

namespace vf {
struct ImageCache {
	enum Flag { eExactExtent = 1 << 0, ePreferHost = 1 << 1, eTrace = 1 << 2 };

	struct Info {
		Vram vram{};
		std::string name{};
		vk::ImageCreateInfo info{};
		vk::ImageAspectFlags aspect{};
		std::uint32_t flags = eExactExtent | ePreferHost;
	};

	Info info{};

	UniqueImage image{};
	vk::UniqueImageView view{};

	static constexpr bool sufficient(vk::Extent3D const avail, vk::Extent3D const target) {
		return avail.width >= target.width && avail.height >= target.height && avail.depth >= target.depth;
	}

	static constexpr vk::Extent3D scale2D(vk::Extent3D extent, float value) {
		auto ret = glm::uvec2(glm::vec2(extent.width, extent.height) * value);
		return {ret.x, ret.y, extent.depth};
	}

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

	bool ready(vk::Extent3D const extent, vk::Format const format) const noexcept {
		if (!image || info.info.format != format) { return false; }
		if ((info.flags & eExactExtent) == eExactExtent) {
			return info.info.extent == extent;
		} else {
			return sufficient(info.info.extent, extent);
		}
	}

	bool make(vk::Extent3D const extent, vk::Format const format) {
		info.info.extent = extent;
		info.info.format = format;
		info.vram.device.defer(std::move(image), std::move(view));
		image = info.vram.makeImage(info.info, (info.flags & ePreferHost) == ePreferHost, info.name.c_str());
		if (!image) { return false; }
		view = info.vram.device.makeImageView(image->resource, format, info.aspect);
		if ((info.flags & eTrace) == eTrace) {
			VF_TRACEF("[vf::(internal)] [{}] refreshed to [{}x{}]", info.name, info.info.extent.width, info.info.extent.height);
		}
		return *view;
	}

	VKImage refresh(vk::Extent3D extent, float const scale = 1.0f, vk::Format format = {}) {
		extent = scale2D(extent, scale);
		if (format == vk::Format()) { format = info.info.format; }
		if (!ready(extent, format)) { make(extent, format); }
		return peek();
	}

	VKImage peek() const noexcept { return {image->resource, view ? *view : vk::ImageView(), {info.info.extent.width, info.info.extent.height}}; }
};

enum class BufferType { eVertex, eIndex, eUniform, eStorage };

struct BufferCache {
	std::string name{};
	mutable vk::BufferCreateInfo info{};
	mutable Rotator<UniqueBuffer, 4> buffers{};
	std::vector<std::byte> data{std::byte{}};
	Vram const* vram{};

	BufferCache() = default;

	BufferCache(Vram const& vram, std::size_t buffering, BufferType type) : vram(&vram) {
		switch (type) {
		case BufferType::eIndex: setIbo(buffering); break;
		case BufferType::eVertex: setVbo(buffering); break;
		default: assert(false && "Invalid buffer type"); break;
		}
	}

	BufferCache(Vram const& vram, BufferType type) : vram(&vram) {
		switch (type) {
		case BufferType::eStorage: setSsbo(); break;
		case BufferType::eUniform: setUbo(); break;
		default: assert(false && "Invalid buffer type"); break;
		}
	}

	explicit operator bool() const { return vram && !buffers.storage.empty(); }

	void init(vk::BufferUsageFlags usage, std::size_t rotations) {
		if (!vram) { return; }
		info.usage = usage;
		info.size = 1;
		for (std::size_t i = 0; i < rotations; ++i) { buffers.push(vram->makeBuffer(info, true, this->name.c_str())); }
	}

	void setVbo(std::size_t buffering) { init(vk::BufferUsageFlagBits::eVertexBuffer, buffering); }
	void setIbo(std::size_t buffering) { init(vk::BufferUsageFlagBits::eIndexBuffer, buffering); }
	void setUbo() { init(vk::BufferUsageFlagBits::eUniformBuffer, 1); }
	void setSsbo() { init(vk::BufferUsageFlagBits::eStorageBuffer, 1); }

	VmaBuffer const& get(bool next) const {
		static auto const blank_v = VmaBuffer{};
		if (!vram) { return blank_v; }
		auto& buffer = buffers.get();
		if (buffers.get()->size < data.size()) {
			info.size = data.size();
			vram->device.defer(std::move(buffer));
			buffer = vram->makeBuffer(info, true, name.c_str());
		}
		buffer->write(std::span<std::byte const>(data));
		if (next) { buffers.next(); }
		return buffer;
	}
};
} // namespace vf
