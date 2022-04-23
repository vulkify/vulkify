#pragma once
#include <vk_mem_alloc.h>
#include <ktl/enum_flags/enum_flags.hpp>
#include <vulkan/vulkan.hpp>
#include <vulkify/core/unique.hpp>

namespace vf {
enum class BlitFlag { eSrc, eDst, eLinearFilter };
using BlitFlags = ktl::enum_flags<BlitFlag, std::uint8_t>;

struct BlitCaps {
	BlitFlags optimal;
	BlitFlags linear;

	static BlitCaps make(vk::PhysicalDevice device, vk::Format format);
};

struct Vram {
	VmaAllocator allocator{};
	vk::Device device{};

	bool operator==(Vram const& rhs) const { return device == rhs.device && allocator == rhs.allocator; }

	struct Deleter {
		void operator()(Vram const& vram) const;
	};
};

using UniqueVram = Unique<Vram, Vram::Deleter>;
UniqueVram makeVram(vk::Instance instance, vk::PhysicalDevice pd, vk::Device device);
} // namespace vf
