#pragma once
#include <vulkify/core/ptr.hpp>

namespace vf {
struct GfxAllocation;

struct TextureHandle {
	Ptr<GfxAllocation const> allocation{};

	bool operator==(TextureHandle const& rhs) const = default;
	explicit constexpr operator bool() const { return allocation != nullptr; }
};
} // namespace vf
