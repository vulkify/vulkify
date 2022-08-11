#pragma once
#include <detail/cache.hpp>
#include <detail/vram.hpp>
#include <glm/mat4x4.hpp>
#include <vulkify/instance/instance_enums.hpp>

namespace vf {
struct PipelineFactory;
class Texture;

inline vk::SamplerCreateInfo sampler_info(Vram const& vram, vk::SamplerAddressMode mode, vk::Filter filter) {
	auto ret = vk::SamplerCreateInfo{};
	ret.minFilter = ret.magFilter = filter;
	ret.anisotropyEnable = vram.device_limits.maxSamplerAnisotropy > 0.0f;
	ret.maxAnisotropy = vram.device_limits.maxSamplerAnisotropy;
	ret.borderColor = vk::BorderColor::eIntOpaqueBlack;
	ret.mipmapMode = vk::SamplerMipmapMode::eNearest;
	ret.addressModeU = ret.addressModeV = ret.addressModeW = mode;
	return ret;
}

constexpr vk::PresentModeKHR from_vsync(VSync vsync) {
	switch (vsync) {
	default:
	case VSync::eOn: return vk::PresentModeKHR::eFifo;
	case VSync::eAdaptive: return vk::PresentModeKHR::eFifoRelaxed;
	case VSync::eTripleBuffer: return vk::PresentModeKHR::eMailbox;
	case VSync::eOff: return vk::PresentModeKHR::eImmediate;
	}
}

constexpr VSync to_vsync(vk::PresentModeKHR mode) {
	switch (mode) {
	default:
	case vk::PresentModeKHR::eFifo: return VSync::eOn;
	case vk::PresentModeKHR::eFifoRelaxed: return VSync::eAdaptive;
	case vk::PresentModeKHR::eMailbox: return VSync::eTripleBuffer;
	case vk::PresentModeKHR::eImmediate: return VSync::eOff;
	}
}

struct ImageSampler {
	ImageCache cache{};
	vk::UniqueSampler sampler{};

	bool operator==(ImageSampler const& rhs) const { return !sampler && !rhs.sampler && !cache.image && !rhs.cache.image; }
};

struct GfxAllocation {
	BufferCache buffers[2]{};
	ImageSampler image{};
	Vram vram{};

	GfxAllocation() = default;
	GfxAllocation(Vram const& vram) : image{{{vram}}}, vram(vram) {}

	bool operator==(GfxAllocation const& rhs) const { return !image.sampler && !rhs.image.sampler; }

	void replace(ImageCache&& cache) {
		vram.device.defer(std::move(image.cache));
		image.cache = std::move(cache);
	}
};

struct GfxCommandBuffer {
	Vram vram;
	CommandFactory::Scoped pool;
	vk::CommandBuffer cmd;
	ImageWriter writer;

	GfxCommandBuffer(Vram const& vram) : vram(vram), pool(*vram.command_factory), cmd(pool.get().acquire()), writer(vram, cmd) {}
	~GfxCommandBuffer() { pool.get().release(std::move(cmd), true); }

	GfxCommandBuffer& operator=(GfxCommandBuffer&&) = delete;
};

struct GfxShaderModule {
	vk::UniqueShaderModule module{};
	vk::Device device{};
};

struct CombinedImageSampler {
	vk::ImageView view{};
	vk::Sampler sampler{};

	constexpr bool operator==(CombinedImageSampler const& rhs) const { return view == rhs.view && sampler == rhs.sampler; }
};

struct Inactive {
	Vram vram{};
	GfxAllocation alloc{};
	std::string name{};
};

Inactive const g_inactive{};
} // namespace vf
