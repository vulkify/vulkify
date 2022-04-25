#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <algorithm>
#include <vector>

namespace vf {
template <typename T, std::size_t Size>
struct BasePimpl {
	ktl::fixed_pimpl<T, Size> storage{};

	template <std::derived_from<T> U>
		requires(sizeof(U) <= Size)
	BasePimpl(U&& u) : storage(std::forward<U>(u)) {}

	T const& get() const { return storage.get(); }
	T& get() { return storage.get(); }
};

template <std::size_t Size>
struct DeferQueue {
	struct Base {
		int delay;
		Base(int delay = default_delay_v) : delay(delay) {}
		virtual ~Base() = default;
	};
	template <typename T>
	struct Model : Base {
		T t;
		Model(T t, int delay) : Base(delay), t(std::move(t)) {}
	};
	using Entry = BasePimpl<Base, Size>;

	static constexpr int default_delay_v = 3;

	std::vector<Entry> entries{};

	template <typename T>
	void operator()(T t, int delay = default_delay_v) {
		entries.push_back(Entry{Model<T>(std::move(t), delay)});
	}

	void decrement() {
		std::erase_if(entries, [](Entry& e) { return --e.get().delay <= 0; });
	}
};

constexpr std::size_t defer_queue_size_v = 64;
using Defer = DeferQueue<defer_queue_size_v>;
} // namespace vf
