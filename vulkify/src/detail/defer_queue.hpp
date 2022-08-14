#pragma once
#include <detail/defer_base.hpp>
#include <ktl/kunique_ptr.hpp>
#include <mutex>
#include <vector>

namespace vf {
template <typename T>
concept BoolLike = requires(T const& t) {
	static_cast<bool>(t);
};

class DeferQueue {
  public:
	static constexpr auto default_delay_v = DeferBase::default_delay_v;

	DeferQueue() : m_mutex(ktl::make_unique<std::mutex>()) {}

	using Entry = ktl::kunique_ptr<DeferBase>;

	template <BoolLike T>
	void push(T t, int delay = default_delay_v) {
		if (t) {
			auto entry = ktl::make_unique<Model<T>>(std::move(t), delay);
			auto lock = std::scoped_lock(*m_mutex);
			m_entries.push_back(std::move(entry));
		}
	}

	void push_direct(Entry&& entry, int delay = default_delay_v);

	void decrement();
	void clear();

  private:
	template <typename T>
	struct Model : DeferBase {
		T t;
		Model(T&& t, int delay) : DeferBase(delay), t(std::move(t)) {}
	};

	std::vector<Entry> m_entries{};
	std::vector<Entry> m_expired{};
	ktl::kunique_ptr<std::mutex> m_mutex{};
};
} // namespace vf
