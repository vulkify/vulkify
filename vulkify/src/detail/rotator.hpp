#pragma once
#include <ktl/fixed_vector.hpp>

namespace vf {
template <typename T, std::size_t Capacity = 8>
struct Rotator {
	ktl::fixed_vector<T, Capacity> storage{};
	std::size_t index{};

	template <typename... Args>
	T& push(Args&&... args) {
		return storage.emplace_back(std::forward<Args>(args)...);
	}

	T& get() { return storage[index]; }
	T const& get() const { return storage[index]; }

	void next() { index = (index + 1) % storage.size(); }
};
} // namespace vf
