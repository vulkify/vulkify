#pragma once

namespace vf {
template <typename T>
struct DirtyFlag {
	mutable T t{};
	mutable bool dirty{};

	constexpr void setDirty() { dirty = true; }
	constexpr void setClean() const { dirty = false; }
	constexpr T& get() const { return t; }
};
} // namespace vf
