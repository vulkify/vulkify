#pragma once
#include <vk_mem_alloc.h>
#include <ktl/enum_flags/enum_flags.hpp>
#include <vulkan/vulkan.hpp>
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

struct ImgMeta {
	LayerMip layerMip{};
	std::pair<vk::AccessFlags, vk::AccessFlags> access{};
	std::pair<vk::PipelineStageFlags, vk::PipelineStageFlags> stages{};
	std::pair<vk::ImageLayout, vk::ImageLayout> layouts{};
	vk::ImageAspectFlags aspects = vk::ImageAspectFlagBits::eColor;

	void imageBarrier(vk::CommandBuffer cb, vk::Image image) const;
};

struct VmaImage {
	vk::Image image{};
	VmaAllocator allocator{};
	VmaAllocation allocation{};

	bool operator==(VmaImage const&) const = default;

	struct Deleter {
		void operator()(VmaImage const& img) const;
	};
};
using UniqueImage = Unique<VmaImage, VmaImage::Deleter>;

struct Vram {
	VmaAllocator allocator{};
	vk::Device device{};
	std::uint32_t queue{};

	bool operator==(Vram const& rhs) const { return device == rhs.device && allocator == rhs.allocator; }

	UniqueImage makeImage(vk::ImageCreateInfo info, VmaMemoryUsage usage) const;

	struct Deleter {
		void operator()(Vram const& vram) const;
	};
};

using UniqueVram = Unique<Vram, Vram::Deleter>;
UniqueVram makeVram(vk::Instance instance, vk::PhysicalDevice pd, vk::Device device, std::uint32_t queue);
} // namespace vf
