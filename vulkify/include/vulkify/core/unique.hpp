#pragma once
#include <concepts>
#include <utility>

namespace vf {
template <typename T>
concept EqualityComparable = requires(T a, T b) {
	{ a == b } -> std::convertible_to<bool>;
};

///
/// \brief Optional unique owned instance of Type and a custom Deleter
///
template <EqualityComparable Type, typename Deleter = void>
class Unique {
  public:
	constexpr Unique(Type t = {}, Deleter deleter = {}) : m_t(std::move(t)), m_del(std::move(deleter)) {}
	constexpr Unique(Unique&& rhs) noexcept : Unique() { swap(rhs); }
	constexpr Unique& operator=(Unique&& rhs) noexcept {
		swap(rhs);
		return *this;
	}
	constexpr ~Unique() {
		if (m_t != Type{}) { m_del(m_t); }
	}

	constexpr bool operator==(Unique const& rhs) const { return m_t == rhs.m_t; }
	explicit constexpr operator bool() const { return m_t != Type{}; }

	constexpr Type& get() { return m_t; }
	constexpr Type const& get() const { return m_t; }
	constexpr Deleter& deleter() { return m_del; }
	constexpr Deleter const& deleter() const { return m_del; }
	constexpr operator Type&() { return get(); }
	constexpr operator Type const&() const { return get(); }
	constexpr Type* operator->() { return &get(); }
	constexpr Type const* operator->() const { return &get(); }

  private:
	constexpr void swap(Unique& rhs) noexcept {
		std::swap(m_t, rhs.m_t);
		std::swap(m_del, rhs.m_del);
	}

	Type m_t{};
	Deleter m_del{};
};

///
/// \brief Optional unique owned instance of Type
///
template <EqualityComparable Type>
class Unique<Type, void> {
  public:
	constexpr Unique(Type t = {}) : m_t(std::move(t)) {}
	constexpr Unique(Unique&& rhs) noexcept : Unique() { std::exchange(m_t, rhs.m_t); }
	constexpr Unique& operator=(Unique&& rhs) noexcept { return (std::exchange(m_t, rhs.m_t), *this); }

	constexpr bool operator==(Unique const& rhs) const { return m_t == rhs.m_t; }
	explicit constexpr operator bool() const { return m_t != Type{}; }

	constexpr Type& get() { return m_t; }
	constexpr Type const& get() const { return m_t; }
	constexpr operator Type&() { return get(); }
	constexpr operator Type const&() { return get(); }
	constexpr Type* operator->() { return &get(); }
	constexpr Type const* operator->() const { return &get(); }

  private:
	Type m_t{};
};
} // namespace vf
