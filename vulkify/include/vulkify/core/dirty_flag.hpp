#pragma once

namespace vf {
template <typename T>
struct DirtyFlag {
	mutable T t{};
	mutable bool dirty{};

	constexpr void set_dirty() { dirty = true; }
	constexpr void set_clean() const { dirty = false; }
	constexpr T& get() const { return t; }
};
} // namespace vf
