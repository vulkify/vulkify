#pragma once
#include <ktl/enum_flags/enum_flags.hpp>
#include <string>

namespace vf {
struct Gpu {
	enum class Type { eUnknown, eDiscrete, eIntegrated, eCpu, eVirtual, eOther };

	std::string name{};
	float maxLineWidth{};
	Type type{};
};
} // namespace vf
