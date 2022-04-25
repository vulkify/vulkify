#pragma once
#include <ktl/enum_flags/enum_flags.hpp>
#include <memory>
#include <string>

namespace vf {
enum class PipelineFlag { eNoDepthTest, eNoDepthWrite, eNoAlphaBlend, eWireframe };
using PipelineFlags = ktl::enum_flags<PipelineFlag, std::uint8_t>;

struct PipelineSpec {
	std::string vertShader{};
	std::string fragShader{};
	PipelineFlags flags{};
	float lineWidth{1.0f};

	bool operator==(PipelineSpec const&) const = default;
};
} // namespace vf
