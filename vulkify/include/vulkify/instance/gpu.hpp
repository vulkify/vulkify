#pragma once
#include <ktl/enum_flags/enum_flags.hpp>
#include <vulkify/instance/instance_enums.hpp>
#include <string>

namespace vf {
struct Gpu {
	enum class Type { eUnknown, eDiscrete, eIntegrated, eCpu, eVirtual, eOther };
	enum class Feature { eWireframe, eWideLines, eMsaa, eAnisotropicFiltering };
	using Features = ktl::enum_flags<Feature>;
	using PresentModes = ktl::enum_flags<VSync>;

	std::string name{};
	Features features{};
	PresentModes present_modes{};
	float max_line_width{};
	Type type{};
};
} // namespace vf
