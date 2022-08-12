#pragma once
#include <vk_mem_alloc.h>
#include <detail/command_pool2.hpp>
#include <detail/vulkan_device.hpp>
#include <vulkify/core/pool.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/core/rgba.hpp>
#include <vulkify/core/unique.hpp>

struct FT_LibraryRec_;
using FT_Library = FT_LibraryRec_*;

namespace vf::refactor {
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

template <typename T, typename U = T>
using TPair = std::pair<T, U>;

struct ImageBarrier {
	static constexpr auto access_flags_v = vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite;
	static constexpr vk::PipelineStageFlags stage_flags_v = vk::PipelineStageFlagBits::eAllCommands;

	TPair<vk::AccessFlags> access{access_flags_v, access_flags_v};
	TPair<vk::PipelineStageFlags> stages{stage_flags_v, stage_flags_v};
	vk::ImageAspectFlags aspects{vk::ImageAspectFlagBits::eColor};
	LayerMip layer_mip{};

	vk::ImageLayout operator()(vk::CommandBuffer cb, vk::Image image, TPair<vk::ImageLayout> layouts) const;
	vk::ImageLayout operator()(vk::CommandBuffer cb, vk::Image image, vk::ImageLayout target) const;
};

template <typename T>
struct VmaResource {
	T resource{};
	VmaAllocator allocator{};
	VmaAllocation handle{};
};

template <typename T>
constexpr bool operator==(VmaResource<T> const& a, VmaResource<T> const& b) {
	return a.allocator == b.allocator && a.handle == b.handle;
}

struct VmaBuffer : VmaResource<vk::Buffer> {
	static constexpr auto full_v = std::numeric_limits<std::size_t>::max();

	std::size_t size{};
	void* map{};

	bool write(void const* data, std::size_t size);

	struct Deleter {
		void operator()(VmaBuffer const&) const;
	};
};

struct VmaImage : VmaResource<vk::Image> {
	vk::ImageLayout layout{};
	vk::Extent3D extent{};
	vk::ImageTiling tiling{};
	BlitCaps caps{};

	void transition(vk::CommandBuffer cb, vk::ImageLayout to, ImageBarrier const& barrier = {});
	ImageView image(vk::ImageView view = {}) const { return {resource, view, {extent.width, extent.height}}; }
	BlitFlags blit_flags() const { return tiling == vk::ImageTiling::eLinear ? caps.linear : caps.optimal; }

	struct Deleter {
		void operator()(VmaImage const&) const;
	};
};

using UniqueImage = Unique<VmaImage, VmaImage::Deleter>;
using UniqueBuffer = Unique<VmaBuffer, VmaBuffer::Deleter>;

struct CommandPoolFactory;
using CommandFactory = Pool<CommandPool, CommandPoolFactory>;

struct GfxDevice {
	VulkanDevice device{};
	VmaAllocator allocator{};
	FT_Library ftlib{};
	std::size_t buffering{};
	CommandFactory* command_factory{};

	vk::PhysicalDeviceLimits const* device_limits{};
	vk::SampleCountFlagBits colour_samples{};
	vk::Format texture_format = vk::Format::eR8G8B8A8Srgb;

	bool operator==(GfxDevice const& rhs) const { return command_factory == rhs.command_factory && allocator == rhs.allocator; }
	explicit operator bool() const { return command_factory && allocator; }

	UniqueImage make_image(vk::ImageCreateInfo info, bool host, bool linear = false) const;
	UniqueBuffer make_buffer(vk::BufferCreateInfo info, bool host) const;
	vk::SamplerCreateInfo sampler_info(vk::SamplerAddressMode mode, vk::Filter filter) const;

	struct Deleter {
		void operator()(GfxDevice const& vram) const;
	};
};

struct UniqueGfxDevice {
	ktl::kunique_ptr<CommandFactory> commandFactory{};
	Unique<GfxDevice, GfxDevice::Deleter> device{};

	explicit operator bool() const { return device && commandFactory; }

	static UniqueGfxDevice make(vk::Instance instance, VulkanDevice device, FT_Library ft, int samples);
};
} // namespace vf::refactor
