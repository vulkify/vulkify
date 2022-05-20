#pragma once
#include <detail/cache.hpp>
#include <detail/vram.hpp>
#include <glm/mat4x4.hpp>

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
