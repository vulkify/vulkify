#pragma once
#include <ktl/kunique_ptr.hpp>
#include <algorithm>
#include <vector>

namespace vf {
struct DeferQueue {
	struct Base {
		int delay;
		Base(int delay = default_delay_v) : delay(delay) {}
		virtual ~Base() = default;
	};
	template <typename T>
	struct Model : Base {
		T t;
		Model(T&& t, int delay) : Base(delay), t(std::move(t)) {}
	};
	using Entry = ktl::kunique_ptr<Base>;

	static constexpr int default_delay_v = 3;

	std::vector<Entry> entries{};

	template <typename T>
	void push(T t, int delay = default_delay_v) {
		if (t) { entries.push_back(ktl::make_unique<Model<T>>(std::move(t), delay)); }
	}

	void decrement() {
		std::erase_if(entries, [](Entry& e) { return --e->delay <= 0; });
	}
};

struct Defer {
	DeferQueue* queue{};

	template <typename... T>
	void operator()(T&&... t) const {
		if (queue) { (queue->push(std::forward<T>(t)), ...); }
	}
};
} // namespace vf
