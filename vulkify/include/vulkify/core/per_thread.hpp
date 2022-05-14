#pragma once
#include <ktl/async/kmutex.hpp>
#include <thread>
#include <unordered_map>

namespace vf {
///
/// \brief Prototype generator (customization point)
///
template <typename T>
struct PerThreadFactory {
	T prototype{};
	T operator()() const { return prototype; }
};

///
/// \brief Container of T, one per thread
///
template <typename T, typename Factory = PerThreadFactory<T>>
class PerThread {
  public:
	PerThread(Factory factory = {}) : m_factory(std::move(factory)) {}

	///
	/// \brief Get a reference to T for this thread
	///
	T& get() const {
		auto const id = std::this_thread::get_id();
		auto lock = ktl::klock(m_map);
		auto it = lock->find(id);
		if (it == lock->end()) {
			auto [i, _] = lock->insert_or_assign(id, m_factory());
			it = i;
		}
		return it->second;
	}

	T& operator()() const { return get(); }

	std::size_t size() const {
		auto lock = ktl::klock(m_map);
		return lock->size();
	}

	bool empty() const {
		auto lock = ktl::klock(m_map);
		return lock->empty();
	}

	void clear() {
		auto lock = ktl::klock(m_map);
		lock->clear();
	}

	Factory& factory() { return m_factory; }
	Factory const& factory() const { return m_factory; }

  private:
	using Map = std::unordered_map<std::thread::id, T>;

	mutable ktl::strict_tmutex<Map> m_map{};
	Factory m_factory{};
};
} // namespace vf
