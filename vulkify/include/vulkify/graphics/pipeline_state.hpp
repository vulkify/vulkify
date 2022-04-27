#pragma once
#include <ktl/enum_flags/enum_flags.hpp>

namespace vf {
struct PipelineState {
	enum class Flag { eNoDepthTest, eNoDepthWrite, eNoAlphaBlend, eWireframe };
	using Flags = ktl::enum_flags<Flag, std::uint8_t>;

	Flags flags{};
	float lineWidth{1.0f};

	bool operator==(PipelineState const&) const = default;
};
} // namespace vf
