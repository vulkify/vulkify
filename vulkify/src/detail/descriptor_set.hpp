#pragma once
#include <detail/rotator.hpp>
#include <detail/trace.hpp>
#include <detail/vram.hpp>
#include <ktl/enumerate.hpp>
#include <ktl/fixed_vector.hpp>
#include <array>
#include <vector>

namespace vf {
struct DescriptorSet {
	static constexpr std::size_t max_bindings_v = 8;
	inline static std::uint32_t s_ssbo_binding = 1;

	Vram* vram{};
	std::array<UniqueBuffer, max_bindings_v>* buffers{};
	vk::DescriptorSet set{};
	char const* name = "";
	std::uint32_t number{};

	explicit operator bool() const { return vram && *vram && buffers && set; }

	vk::BufferUsageFlagBits usage(std::uint32_t const binding) const {
		return binding == s_ssbo_binding ? vk::BufferUsageFlagBits::eStorageBuffer : vk::BufferUsageFlagBits::eUniformBuffer;
	}

	vk::DescriptorType type(std::uint32_t const binding) const {
		return binding == s_ssbo_binding ? vk::DescriptorType::eStorageBuffer : vk::DescriptorType::eUniformBuffer;
	}

	void refresh(UniqueBuffer& out, std::size_t const size, vk::BufferUsageFlagBits const usage) const {
		if (!out->resource || out->size < size) { out = vram->makeBuffer({{}, size, usage}, VMA_MEMORY_USAGE_CPU_ONLY, name); }
	}

	bool write(std::uint32_t binding, void const* data, std::size_t size) {
		if (!static_cast<bool>(*this)) { return false; }
		refresh(buffers->at(binding), size, usage(binding));
		auto buf = buffers->at(binding).get();
		bool const ret = buf.write(data, size);
		auto dbi = vk::DescriptorBufferInfo(buf.resource, {}, size);
		auto wds = vk::WriteDescriptorSet(set, binding, 0, 1, type(binding), {}, &dbi);
		vram->device.device.updateDescriptorSets(1, &wds, 0, {});
		return ret;
	}

	template <typename T>
		requires(std::is_standard_layout_v<T>)
	bool write(std::uint32_t binding, T const& t) { return write(binding, &t, sizeof(T)); }

	void bind(vk::CommandBuffer cmd, vk::PipelineLayout const layout) const {
		if (!set || !cmd || !layout) { return; }
		cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, number, set, {});
	}
};

struct DescriptorPool {
	static constexpr std::size_t max_sets_v = 8;

	struct Set {
		vk::DescriptorSet set{};
		std::uint32_t number{};
		std::array<UniqueBuffer, DescriptorSet::max_bindings_v> buffers{};
	};

	struct Pool {
		Vram vram{};
		vk::DescriptorSetLayout layout{};
		vk::UniqueDescriptorPool pool{};
		std::uint32_t number{};
		std::vector<Set> sets{};
		std::size_t index{};

		void reserve(std::size_t size) {
			if (sets.size() < size) {
				auto const count = size - sets.size();
				auto layouts = std::vector<vk::DescriptorSetLayout>(count, layout);
				auto info = vk::DescriptorSetAllocateInfo(*pool, static_cast<std::uint32_t>(count), layouts.data());
				auto alloc = vram.device.device.allocateDescriptorSets(info);
				sets.reserve(sets.size() + alloc.size());
				for (auto& set : alloc) { sets.push_back({std::move(set), number}); }
			}
		}

		DescriptorSet get(char const* name) {
			reserve(index + 1);
			auto& set = sets[index];
			return {&vram, &set.buffers, set.set, name, number};
		}
	};
	using Storage = ktl::fixed_vector<Pool, max_sets_v>;

	Rotator<Storage, 8> pools{};
	std::vector<vk::DescriptorSetLayout> layouts{};

	static constexpr vk::DescriptorPoolSize const pool_sizes_v[] = {
		{vk::DescriptorType::eUniformBuffer, 4},
		{vk::DescriptorType::eCombinedImageSampler, 4},
	};

	static DescriptorPool make(Vram vram, std::vector<vk::DescriptorSetLayout> layouts, std::size_t buffering) {
		auto ret = DescriptorPool{};
		for (std::size_t frame = 0; frame < buffering; ++frame) {
			auto storage = Storage{};
			for (auto const& [layout, number] : ktl::enumerate<std::uint32_t>(layouts)) {
				auto info = vk::DescriptorPoolCreateInfo({}, 1024, static_cast<std::uint32_t>(std::size(pool_sizes_v)), pool_sizes_v);
				storage.push_back({vram, layout, vram.device.device.createDescriptorPoolUnique(info), number});
			}
			ret.pools.push(std::move(storage));
		}
		ret.layouts = std::move(layouts);
		return ret;
	}

	explicit operator bool() const { return !layouts.empty(); }

	Pool* pool(std::uint32_t set) {
		auto& pool = pools.get();
		if (set > pool.size()) {
			VF_TRACEF("[DescriptorSet] Invalid set [{}]!", set);
			return {};
		}
		return &pool[set];
	}

	DescriptorSet get(std::uint32_t set, std::size_t index, char const* name) {
		if (auto p = pool(set)) {
			p->index = index;
			return p->get(name);
		}
		return {};
	}

	DescriptorSet postInc(std::uint32_t set, char const* name) {
		if (auto p = pool(set)) {
			auto ret = p->get(name);
			++p->index;
			return ret;
		}
		return {};
	}

	void next() {
		for (auto& pool : pools.get()) { pool.index = {}; }
		pools.next();
	}
};
} // namespace vf
