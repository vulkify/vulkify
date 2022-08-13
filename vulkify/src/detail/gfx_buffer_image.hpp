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

	UniqueImage image{};
	vk::UniqueImageView view{};
	GfxDevice const* device{};

	static constexpr Extent scale2D(Extent extent, float value) { return glm::vec2(extent) * value; }

	explicit operator bool() const { return device && device->device.device; }
	bool operator==(ImageCache const& rhs) const { return !image && !view && !rhs.image && !rhs.view; }

	vk::ImageCreateInfo& set_depth();
	vk::ImageCreateInfo& set_colour();
	vk::ImageCreateInfo& set_texture(bool const transfer_src);

	Extent current() const { return {info.info.extent.width, info.info.extent.height}; }

	bool ready(Extent const extent, vk::Format const format) const noexcept;

	bool make(Extent const extent, vk::Format const format);
	ImageView refresh(Extent const extent, vk::Format format = {});

	ImageView peek() const noexcept { return {image->resource, view ? *view : vk::ImageView(), {info.info.extent.width, info.info.extent.height}}; }
};

struct BufferCache {
	GfxDevice const* device{};
	mutable vk::BufferCreateInfo info{};
	mutable Rotator<UniqueBuffer, 4> buffers{};
	std::vector<std::byte> data{std::byte{}};

	BufferCache() = default;
	BufferCache(GfxDevice const* device, vk::BufferUsageFlagBits usage);

	explicit operator bool() const { return device && !buffers.storage.empty(); }

	VmaBuffer const& get(bool next) const;
};

struct VulkanImage {
	ImageCache cache{};
	vk::UniqueSampler sampler{};

	static VulkanImage make(GfxDevice const* device) { return VulkanImage{.cache = ImageCache{.device = device}}; }

	bool operator==(VulkanImage const& rhs) const { return !sampler && !rhs.sampler && !cache.image && !rhs.cache.image; }
};

template <std::size_t Count>
class GfxBuffer : public GfxAllocation {
  public:
	GfxBuffer(GfxDevice const* device) : GfxAllocation(device, Type::eBuffer) {
		for (auto& buffer : buffers) { buffer.device = device; }
	}

	BufferCache buffers[Count]{};
};

class GfxGeometryBuffer : public GfxBuffer<2> {
  public:
	using GfxBuffer::GfxBuffer;

	std::uint32_t vertices{};
	std::uint32_t indices{};
};

class GfxImage : public GfxAllocation {
  public:
	GfxImage(GfxDevice const* device) : GfxAllocation(device, Type::eImage) { image.cache.device = device; }

	void replace(ImageCache&& cache);

	VulkanImage image{};
};

class GfxShaderModule : public GfxAllocation {
  public:
	GfxShaderModule(GfxDevice const* device) : GfxAllocation(device, Type::eShader) {}

	vk::UniqueShaderModule module{};
};
} // namespace vf::refactor
