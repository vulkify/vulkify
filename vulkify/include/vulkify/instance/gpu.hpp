#pragma once
#include <ktl/enum_flags/enum_flags.hpp>
#include <string>

namespace vf {
struct GPU {
	enum class Type { eUnknown, eDiscrete, eIntegrated, eCpu, eVirtual, eOther };

	std::string name{};
	Type type{};
};
} // namespace vf
