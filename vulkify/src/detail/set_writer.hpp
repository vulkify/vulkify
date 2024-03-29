#pragma once
#include <detail/gfx_device.hpp>
#include <span>

namespace vf {
struct SetWriter {
	enum Binding { eUniform = 0, eStorage = 1, eCOUNT_ };

	struct BufferLayout {
		vk::DescriptorType type{};
		vk::BufferUsageFlagBits usage{};
	};

	static constexpr BufferLayout buffer_layouts_v[] = {
		{vk::DescriptorType::eUniformBuffer, vk::BufferUsageFlagBits::eUniformBuffer},
		{vk::DescriptorType::eStorageBuffer, vk::BufferUsageFlagBits::eStorageBuffer},
	};

	GfxDevice* device{};

	std::span<UniqueBuffer> buffers{};
	vk::DescriptorSet set{};
	std::uint32_t number{};

	explicit operator bool() const { return device && *device && !buffers.empty() && set; }

	void refresh(UniqueBuffer& out, std::size_t const size, vk::BufferUsageFlagBits const usage) const {
		if (!out->resource || out->size != size) { out = device->make_buffer({{}, size, usage}, true); }
	}

	bool write(std::uint32_t binding, void const* data, std::size_t size) {
		assert(binding < eCOUNT_);
		if (!static_cast<bool>(*this)) { return false; }
		refresh(buffers[binding], size, buffer_layouts_v[binding].usage);
		auto buf = buffers[binding].get();
		auto const ret = buf.write(data, size);
		auto dbi = vk::DescriptorBufferInfo(buf.resource, {}, size);
		auto wds = vk::WriteDescriptorSet(set, binding, 0, 1, buffer_layouts_v[binding].type, {}, &dbi);
		device->device.device.updateDescriptorSets(1, &wds, 0, {});
		return ret;
	}

	bool update(std::uint32_t binding, vk::Sampler sampler, vk::ImageView view) {
		if (!static_cast<bool>(*this) || !sampler || !view) { return false; }
		auto dii = vk::DescriptorImageInfo(sampler, view, vk::ImageLayout::eShaderReadOnlyOptimal);
		auto wds = vk::WriteDescriptorSet(set, binding, 0, 1, vk::DescriptorType::eCombinedImageSampler, &dii);
		device->device.device.updateDescriptorSets(1, &wds, 0, {});
		return true;
	}

	void bind(vk::CommandBuffer cmd, vk::PipelineLayout const layout) const {
		if (!set || !cmd || !layout) { return; }
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, number, set, {});
	}
};
} // namespace vf
