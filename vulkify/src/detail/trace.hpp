#pragma once

#if defined(VULKIFY_DEBUG_TRACE)
#include <ktl/str_format.hpp>
#include <string_view>

namespace vf {
void trace(std::string_view);
}

#define VF_TRACE(fmt, ...) ::vf::trace(::ktl::str_format(fmt, __VA_ARGS__))

#else
#define VF_TRACE(fmt, ...)
#endif
