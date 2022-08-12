#pragma once
#include <ktl/hash_table.hpp>
#include <vulkify/core/handle.hpp>
#include <vulkify/core/ptr.hpp>
#include <mutex>

namespace vf::refactor {
template <typename Type>
struct HandleMap {
	struct Hasher {
		std::size_t operator()(Handle<Type> const handle) const { return std::hash<typename Handle<Type>::value_type>{}(handle.value); }
	};

	using Map = ktl::hash_table<Handle<Type>, Type, Hasher>;
	using Lock = std::scoped_lock<std::mutex>;

	Map map{};
	std::mutex mutex{};
	Handle<Type> next{};

	Handle<Type> add(Lock const&, Type t) {
		auto const ret = Handle<Type>{++next.value};
		map.emplace(ret, std::move(t));
		return ret;
	}

	void remove(Lock const&, Handle<Type> handle) { map.erase(handle); }
	bool contains(Lock const&, Handle<Type> const handle) const { return map.find(handle) != map.end(); }

	Ptr<Type> find(Lock const&, Handle<Type> handle) const {
		if (auto it = map.find(handle); it != map.end()) { return &it->second; }
		return {};
	}

	std::size_t size(Lock const&) const { return map.size(); }
	void clear(Lock const&) { map.clear(); }
	bool empty(Lock const&) const { return map.empty(); }
};
} // namespace vf::refactor
