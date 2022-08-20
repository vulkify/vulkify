#pragma once
#include <ktl/enum_flags/enum_flags.hpp>
#include <vulkify/graphics/z_order.hpp>

namespace vf {
enum class WindowFlag { eBorderless, eResizable, eFloating, eAutoIconify, eMaximized };
using WindowFlags = ktl::enum_flags<WindowFlag>;

enum class AntiAliasing { eNone, e2x, e4x, e8x, e16x };
enum class VSync { eOn, eAdaptive, eTripleBuffer, eOff };
} // namespace vf
