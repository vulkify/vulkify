#pragma once
#include <ktl/enum_flags/enum_flags.hpp>
#include <string>

namespace vf {
struct Gpu {
	enum class Type { eUnknown, eDiscrete, eIntegrated, eCpu, eVirtual, eOther };
	enum class Feature { eWireframe, eWideLines, eMsaa, eAnisotropicFiltering };
	using Features = ktl::enum_flags<Feature>;

	std::string name{};
	Features features{};
	float maxLineWidth{};
	Type type{};
};
} // namespace vf
