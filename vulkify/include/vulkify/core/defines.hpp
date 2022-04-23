#pragma once

namespace vf {
constexpr bool debug_v =
#if defined(VULKIFY_DEBUG)
	true;
#else
	false;
#endif

constexpr bool debug_trace_v =
#if defined(VULKIFY_DEBUG_TRACE)
	true;
#else
	false;
#endif
} // namespace vf
