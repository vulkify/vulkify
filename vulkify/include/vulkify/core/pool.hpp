#pragma once
#include <ktl/async/kmutex.hpp>
#include <vector>

namespace vf {
template <typename Type>
struct PoolFactory {
	Type operator()() const { return Type{}; }
};

template <typename Type, typename Factory = PoolFactory<Type>>
class Pool {
  public:
	using value_type = Type;
	using factory_type = Factory;
	class Scoped;

	Pool(Factory factory = {}) : m_factory(std::move(factory)) {}

	Type acquire() {
		auto lock = ktl::klock(m_ts);
		if (lock->empty()) { return m_factory(); }
		auto ret = std::move(lock->back());
		lock->pop_back();
		return ret;
	}

	void release(Type&& t) {
		auto lock = ktl::klock(m_ts);
		lock->push_back(std::move(t));
	}

	void clear() {
		auto lock = ktl::klock(m_ts);
		lock->clear();
	}

	Factory& factory() { return m_factory; }
	Factory const& factory() const { return m_factory; }

  private:
	ktl::strict_tmutex<std::vector<Type>> m_ts{};
	Factory m_factory{};
};

template <typename Type, typename Factory>
class Pool<Type, Factory>::Scoped {
  public:
	Scoped(Pool& pool) : m_pool(pool), m_t(m_pool.acquire()) {}
	~Scoped() { m_pool.release(std::move(m_t)); }

	Type& get() { return m_t; }
	Type const& get() const { return m_t; }

  private:
	Pool& m_pool{};
	Type m_t{};
};
} // namespace vf
