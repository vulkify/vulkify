#pragma once
#include <vk_mem_alloc.h>
#include <detail/command_pool.hpp>
#include <detail/vk_device.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <vulkify/core/geometry.hpp>
#include <vulkify/core/per_thread.hpp>
#include <vulkify/core/unique.hpp>

namespace vf {
class CommandPool;

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
	std::size_t size{};
	void* map{};

	template <typename U>
	static constexpr bool false_v = false;
	static constexpr auto full_v = std::numeric_limits<std::size_t>::max();

	bool operator==(VmaResource const&) const = default;

	bool write(void const* data, std::size_t size = full_v) {
		if (size == full_v) { size = this->size; }
		if (!map || size == 0) { return false; }
		std::memcpy(map, data, size);
		return true;
	}

	template <typename U>
		requires(std::is_trivial_v<U>)
	bool writeT(U const& u) const { return write(&u, sizeof(U)); }

	struct Deleter {
		void operator()(VmaResource const&) const;
	};
};
using VmaImage = VmaResource<vk::Image>;
using VmaBuffer = VmaResource<vk::Buffer>;
using UniqueImage = Unique<VmaImage, VmaImage::Deleter>;
using UniqueBuffer = Unique<VmaBuffer, VmaBuffer::Deleter>;

struct VIBuffer {
	enum class Type { eGpuOnly, eCpuToGpu };

	UniqueBuffer vbo{};
	UniqueBuffer ibo{};
	Type type{};
};

struct InstantCommand {
	DeferQueue defer{};
	CommandPool& pool;
	vk::CommandBuffer cmd{};

	InstantCommand(CommandPool& pool) : pool(pool) { record(); }
	~InstantCommand() { submit(); }

	explicit operator bool() const { return cmd; }

	void record() {
		submit();
		cmd = pool.acquire();
	}

	void submit() {
		if (cmd) { pool.release(std::exchange(cmd, vk::CommandBuffer{}), true, std::move(defer)); }
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
	CommandFactory* commandFactory{};

	bool operator==(Vram const& rhs) const { return commandFactory == rhs.commandFactory && allocator == rhs.allocator; }
	explicit operator bool() const { return commandFactory && allocator; }

	UniqueImage makeImage(vk::ImageCreateInfo info, VmaMemoryUsage usage, char const* name, bool linear = false) const;
	UniqueBuffer makeBuffer(vk::BufferCreateInfo info, VmaMemoryUsage usage, char const* name) const;

	VIBuffer makeVIBuffer(Geometry const& geometry, VIBuffer::Type type, char const* name) const;

	struct Deleter {
		void operator()(Vram const& vram) const;
	};
};

struct UniqueVram {
	ktl::kunique_ptr<CommandFactory> commandFactory{};
	Unique<Vram, Vram::Deleter> vram{};

	explicit operator bool() const { return vram && commandFactory; }

	static UniqueVram make(vk::Instance instance, VKDevice device);
};

template <typename T>
void VmaResource<T>::Deleter::operator()(VmaResource const& resource) const {
	if (resource.map) { vmaUnmapMemory(resource.allocator, resource.handle); }
	if constexpr (std::is_same_v<T, vk::Image>) {
		vmaDestroyImage(resource.allocator, resource.resource, resource.handle);
	} else if constexpr (std::is_same_v<T, vk::Buffer>) {
		vmaDestroyBuffer(resource.allocator, resource.resource, resource.handle);
	} else {
		static_assert(false_v<T>, "Invalid type");
	}
}
} // namespace vf
