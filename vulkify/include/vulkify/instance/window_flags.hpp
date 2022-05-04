#pragma once
#include <ktl/enum_flags/enum_flags.hpp>

namespace vf {
enum class WindowFlag { eBorderless, eResizable, eFloating, eAutoIconify, eMaximized };
using WindowFlags = ktl::enum_flags<WindowFlag>;
} // namespace vf
