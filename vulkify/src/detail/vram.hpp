#pragma once
#include <vk_mem_alloc.h>
#include <detail/command_pool.hpp>
#include <detail/vk_device.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/core/per_thread.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/core/unique.hpp>
#include <vulkify/graphics/geometry.hpp>

struct FT_LibraryRec_;
using FT_Library = FT_LibraryRec_*;

namespace vf {
class CommandPool;
struct ShaderCache;

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

	TPair<vk::ImageLayout> layouts{};
	TPair<vk::AccessFlags> access{access_flags_v, access_flags_v};
	TPair<vk::PipelineStageFlags> stages{stage_flags_v, stage_flags_v};
	vk::ImageAspectFlags aspects{vk::ImageAspectFlagBits::eColor};
	LayerMip layerMip{};

	void operator()(vk::CommandBuffer cb, vk::Image image) const;
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

	bool write(void const* data, std::size_t size = full_v);

	struct Deleter {
		void operator()(VmaBuffer const&) const;
	};
};

struct VmaImage : VmaResource<vk::Image> {
	vk::ImageLayout layout{};
	vk::Extent3D extent{};
	vk::ImageTiling tiling{};
	BlitCaps caps{};

	void transition(vk::CommandBuffer cb, vk::ImageLayout to, ImageBarrier barrier = {});
	VKImage image(vk::ImageView view = {}) const { return {resource, view, {extent.width, extent.height}}; }
	BlitFlags blitFlags() const { return tiling == vk::ImageTiling::eLinear ? caps.linear : caps.optimal; }

	struct Deleter {
		void operator()(VmaImage const&) const;
	};
};

using UniqueImage = Unique<VmaImage, VmaImage::Deleter>;
using UniqueBuffer = Unique<VmaBuffer, VmaBuffer::Deleter>;

struct BufferCache {
	enum class Type { eGpuOnly, eCpuToGpu };

	static constexpr std::size_t max_buffers_v = 4;

	ktl::fixed_vector<UniqueBuffer, max_buffers_v> buffers{};
	Type type{};

	explicit operator bool() const { return !buffers.empty(); }
	bool operator==(BufferCache const& rhs) const { return buffers.empty() && rhs.buffers.empty(); }
};

struct InstantCommand {
	CommandPool* pool{};
	vk::CommandBuffer cmd{};

	InstantCommand(CommandPool& pool) : pool(&pool) { record(); }
	~InstantCommand() { submit(); }

	explicit operator bool() const { return cmd; }

	void record() {
		submit();
		cmd = pool->acquire();
	}

	void submit() {
		if (cmd) { pool->release(std::exchange(cmd, vk::CommandBuffer{}), true); }
	}
};

struct CommandFactory {
	struct Factory {
		VKDevice device{};
		CommandPool operator()() const { return device; }
	};

	PerThread<CommandPool, Factory> commandPools{};
	CommandPool& get() const { return commandPools.get(); }
};

struct Vram {
	VKDevice device{};
	VmaAllocator allocator{};
	FT_Library ftlib{};
	CommandFactory* commandFactory{};
	ShaderCache* shaderCache{};

	float maxAnisotropy{};
	vk::SampleCountFlagBits colourSamples{};
	TPair<float> lineWidthLimit{};
	vk::Format textureFormat = vk::Format::eR8G8B8A8Srgb;

	bool operator==(Vram const& rhs) const { return commandFactory == rhs.commandFactory && allocator == rhs.allocator; }
	explicit operator bool() const { return commandFactory && allocator; }

	UniqueImage makeImage(vk::ImageCreateInfo info, bool host, char const* name, bool linear = false) const;
	UniqueBuffer makeBuffer(vk::BufferCreateInfo info, bool host, char const* name) const;

	BufferCache makeVIBuffer(Geometry const& geometry, BufferCache::Type type, char const* name) const;

	struct Deleter {
		void operator()(Vram const& vram) const;
	};
};

struct ImageWriter {
	using Rect = vf::TRect<std::uint32_t>;
	using Offset = vf::TRect<std::int32_t>;

	Vram const* vram{};
	vk::CommandBuffer cb;

	ImageWriter(Vram const& vram, vk::CommandBuffer cb) : vram(&vram), cb(cb) {}

	std::vector<UniqueBuffer> scratch{};

	static bool canBlit(VmaImage const& src, VmaImage const& dst);

	bool write(VmaImage& out, std::span<std::byte const> data, Rect rect = {}, vk::ImageLayout il = {});
	bool blit(VmaImage& in, VmaImage& out, Offset const& inr, Offset const& outr, vk::Filter filter, TPair<vk::ImageLayout> il = {}) const;
	bool copy(VmaImage& in, VmaImage& out, Offset const& inr, Offset const& outr, TPair<vk::ImageLayout> il = {}) const;
	void clear(VmaImage& in, Rgba rgba) const;
};

struct UniqueVram {
	ktl::kunique_ptr<CommandFactory> commandFactory{};
	Unique<Vram, Vram::Deleter> vram{};

	explicit operator bool() const { return vram && commandFactory; }

	static UniqueVram make(vk::Instance instance, VKDevice device, int samples);
};
} // namespace vf
