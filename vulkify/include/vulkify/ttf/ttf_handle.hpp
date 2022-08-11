#pragma once
#include <vulkify/core/ptr.hpp>

namespace vf {
struct GfxFont;

struct TtfHandle {
	Ptr<GfxFont> font{};

	bool operator==(TtfHandle const& rhs) const = default;
	explicit constexpr operator bool() const { return font != nullptr; }
};
} // namespace vf
