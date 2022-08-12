#pragma once
#include <vulkify/core/ptr.hpp>

namespace vf::refactor {
class GfxAllocation;

template <typename Type>
struct Handle {
	Ptr<GfxAllocation const> allocation{};

	bool operator==(Handle const& rhs) const = default;
	explicit constexpr operator bool() const { return allocation != nullptr; }
};
} // namespace vf::refactor
