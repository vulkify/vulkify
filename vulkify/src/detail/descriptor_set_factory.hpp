#pragma once
#include <detail/descriptor_set.hpp>
#include <detail/rotator.hpp>
#include <detail/trace.hpp>
#include <ktl/enumerate.hpp>
#include <ktl/fixed_vector.hpp>
#include <vector>

namespace vf {
struct DescriptorAllocator {
	struct Set {
		UniqueBuffer buffers[DescriptorSet::Binding::eCOUNT_]{};
		vk::DescriptorSet set{};
	};
	struct Pool {
		std::vector<Set> sets{};
		std::vector<vk::UniqueDescriptorPool> descriptorPools{};
	};

	static constexpr std::size_t block_size_v = 64;

	Vram vram{};
	vk::DescriptorSetLayout layout{};
	std::uint32_t number{};

	Rotator<Pool, 4> pools{};
	std::size_t index{};

	static DescriptorAllocator make(Vram const& vram, vk::DescriptorSetLayout layout, std::uint32_t number) {
		auto ret = DescriptorAllocator{vram, layout, number};
		for (std::size_t i = 0; i < vram.buffering; ++i) {
			auto pool = Pool{};
			pool.descriptorPools.push_back(ret.makeDescriptorPool());
			ret.pools.push(std::move(pool));
		}
		return ret;
	}

	vk::UniqueDescriptorPool makeDescriptorPool() const {
		if (!vram) { return {}; }
		vk::DescriptorPoolSize const poolSizes[] = {
			{vk::DescriptorType::eUniformBuffer, block_size_v},
			{vk::DescriptorType::eStorageBuffer, block_size_v},
			{vk::DescriptorType::eCombinedImageSampler, block_size_v},
		};
		auto dpci = vk::DescriptorPoolCreateInfo({}, block_size_v, static_cast<std::uint32_t>(std::size(poolSizes)), poolSizes);
		return vram.device.device.createDescriptorPoolUnique(dpci);
	}

	bool tryAllocate(vk::DescriptorPool pool, Pool& out) const {
		vk::DescriptorSetLayout layouts[block_size_v]{};
		std::fill(std::begin(layouts), std::end(layouts), layout);
		auto dsai = vk::DescriptorSetAllocateInfo(pool, static_cast<std::uint32_t>(std::size(layouts)), layouts);
		vk::DescriptorSet dsets[block_size_v]{};
		auto res = vram.device.device.allocateDescriptorSets(&dsai, dsets);
		if (res != vk::Result::eSuccess) { return false; }
		out.sets.reserve(out.sets.size() + block_size_v);
		for (auto set : dsets) { out.sets.push_back(Set{{}, set}); }
		return true;
	}

	void allocate() {
		auto& storage = pools.get();
		assert(!storage.descriptorPools.empty());
		auto pool = &storage.descriptorPools.back();
		if (!tryAllocate(pool->get(), storage)) {
			storage.descriptorPools.push_back(makeDescriptorPool());
			pool = &storage.descriptorPools.back();
			[[maybe_unused]] auto const res = tryAllocate(pool->get(), storage);
			assert(res);
		}
	}

	Set& set() {
		auto& storage = pools.get();
		storage.sets.reserve(index + 1);
		while (storage.sets.size() <= index) { allocate(); }
		return storage.sets[index];
	}

	DescriptorSet descriptorSet(char const* name) {
		auto& set = this->set();
		return {&vram, set.buffers, set.set, name, number};
	}

	void next() {
		pools.next();
		index = {};
	}
};

struct DescriptorSetFactory {
	static constexpr std::size_t sets_v = 3;

	DescriptorAllocator allocators[sets_v]{};

	static DescriptorSetFactory make(Vram const& vram, std::span<vk::DescriptorSetLayout const> layouts) {
		auto ret = DescriptorSetFactory{};
		for (auto const [layout, number] : ktl::enumerate<std::uint32_t>(layouts)) { ret.allocators[number] = DescriptorAllocator::make(vram, layout, number); }
		return ret;
	}

	explicit operator bool() const { return !allocators[0].pools.storage.empty(); }

	DescriptorSet postInc(std::uint32_t set, char const* name) {
		assert(set < sets_v);
		auto& rot = allocators[set];
		auto ret = rot.descriptorSet(name);
		++rot.index;
		return ret;
	}

	void next() {
		for (auto& rotator : allocators) { rotator.next(); }
	}
};
} // namespace vf
