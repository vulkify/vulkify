#pragma once

#if defined(VULKIFY_DEBUG_TRACE)
#include <ktl/str_format.hpp>

namespace vf {
void trace(std::string);
}

#define VF_TRACE(msg) ::vf::trace(msg)
#define VF_TRACEF(fmt, ...) ::vf::trace(::ktl::str_format(fmt, __VA_ARGS__))

#else
#define VF_TRACE(unused)
#define VF_TRACEF(unused, ...)
#endif
