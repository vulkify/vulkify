#pragma once
#include <detail/cache.hpp>
#include <detail/vram.hpp>
#include <glm/mat4x4.hpp>
#include <vulkify/instance/instance_enums.hpp>

namespace vf {
struct PipelineFactory;
class Texture;

inline vk::SamplerCreateInfo samplerInfo(Vram const& vram, vk::SamplerAddressMode mode, vk::Filter filter) {
	auto ret = vk::SamplerCreateInfo{};
	ret.minFilter = ret.magFilter = filter;
	ret.anisotropyEnable = vram.deviceLimits.maxSamplerAnisotropy > 0.0f;
	ret.maxAnisotropy = vram.deviceLimits.maxSamplerAnisotropy;
	ret.borderColor = vk::BorderColor::eIntOpaqueBlack;
	ret.mipmapMode = vk::SamplerMipmapMode::eNearest;
	ret.addressModeU = ret.addressModeV = ret.addressModeW = mode;
	return ret;
}

constexpr vk::PresentModeKHR fromVSync(VSync vsync) {
	switch (vsync) {
	default:
	case VSync::eOn: return vk::PresentModeKHR::eFifo;
	case VSync::eAdaptive: return vk::PresentModeKHR::eFifoRelaxed;
	case VSync::eTripleBuffer: return vk::PresentModeKHR::eMailbox;
	case VSync::eOff: return vk::PresentModeKHR::eImmediate;
	}
}

constexpr VSync toVSync(vk::PresentModeKHR mode) {
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
	std::string name{};

	GfxAllocation() = default;
	GfxAllocation(Vram const& vram, std::string name) : image{{{vram, name}}}, vram(vram), name(std::move(name)) {}

	bool operator==(GfxAllocation const& rhs) const { return !image.sampler && !rhs.image.sampler; }

	void replace(ImageCache&& cache) {
		vram.device.defer(std::move(image.cache));
		image.cache = std::move(cache);
	}
};

struct GfxShaderModule {
	vk::UniqueShaderModule module{};
	vk::Device device{};
};

struct GfxCommandBuffer {
	CommandPool& pool;
	vk::CommandBuffer cmd;
	ImageWriter writer;

	GfxCommandBuffer(Vram const& vram) : pool(vram.commandFactory->get()), cmd(pool.acquire()), writer(vram, cmd) {}
	~GfxCommandBuffer() { pool.release(std::move(cmd), true); }
};

struct Inactive {
	Vram vram{};
	GfxAllocation alloc{};
	std::string name{};
};

Inactive const g_inactive{};
} // namespace vf
