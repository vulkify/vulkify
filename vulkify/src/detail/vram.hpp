#pragma once
#include <vk_mem_alloc.h>
#include <detail/vk_device.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <vulkify/core/geometry.hpp>
#include <vulkify/core/unique.hpp>

namespace vf {
enum class BlitFlag { eSrc, eDst, eLinearFilter };
using BlitFlags = ktl::enum_flags<BlitFlag, std::uint8_t>;

struct BlitCaps {
	BlitFlags optimal{};
	BlitFlags linear{};

	static BlitCaps make(vk::PhysicalDevice device, vk::Format format);
};

struct LayerMip {
	struct Range {
		std::uint32_t first = 0U;
		std::uint32_t count = 1U;
	};

	Range layer{};
	Range mip{};

	static LayerMip make(std::uint32_t mipCount, std::uint32_t firstMip = 0U) noexcept { return LayerMip{{}, {firstMip, mipCount}}; }
};

struct ImageBarrier {
	LayerMip layerMip{};
	std::pair<vk::AccessFlags, vk::AccessFlags> access{};
	std::pair<vk::PipelineStageFlags, vk::PipelineStageFlags> stages{};
	std::pair<vk::ImageLayout, vk::ImageLayout> layouts{};
	vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor;

	void operator()(vk::CommandBuffer cb, vk::Image image) const;
};

template <typename T>
struct VmaResource {
	T resource{};
	VmaAllocator allocator{};
	VmaAllocation handle{};

	template <typename U>
	static constexpr bool false_v = false;

	bool operator==(VmaResource const&) const = default;

	struct Deleter {
		void operator()(VmaResource const&) const;
	};
};
using VmaImage = VmaResource<vk::Image>;
using VmaBuffer = VmaResource<vk::Buffer>;
using UniqueImage = Unique<VmaImage, VmaImage::Deleter>;
using UniqueBuffer = Unique<VmaBuffer, VmaBuffer::Deleter>;

struct Vram {
	VKDevice device{};
	VmaAllocator allocator{};

	bool operator==(Vram const& rhs) const { return device.device == rhs.device.device && allocator == rhs.allocator; }
	explicit operator bool() const { return device.device && allocator; }

	UniqueImage makeImage(vk::ImageCreateInfo info, VmaMemoryUsage usage, bool linear = false) const;
	UniqueBuffer makeBuffer(vk::BufferCreateInfo info, VmaMemoryUsage usage) const;

	UniqueBuffer makeVBO(Geometry const& geometry) const;

	struct Deleter {
		void operator()(Vram const& vram) const;
	};
};

using UniqueVram = Unique<Vram, Vram::Deleter>;
UniqueVram makeVram(vk::Instance instance, VKDevice device);

template <typename T>
void VmaResource<T>::Deleter::operator()(VmaResource const& resource) const {
	if constexpr (std::is_same_v<T, vk::Image>) {
		vmaDestroyImage(resource.allocator, resource.resource, resource.handle);
	} else if constexpr (std::is_same_v<T, vk::Buffer>) {
		vmaDestroyBuffer(resource.allocator, resource.resource, resource.handle);
	} else {
		static_assert(false_v<T>, "Invalid type");
	}
}
} // namespace vf
