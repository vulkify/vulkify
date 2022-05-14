#pragma once

namespace vf {
///
/// \brief True when VULKIFY_DEBUG is defined (in Debug configurations by default)
///
constexpr bool debug_v =
#if defined(VULKIFY_DEBUG)
	true;
#else
	false;
#endif

///
/// \brief True when VULKIFY_DEBUG_TRACE is defined (driven by CMake option)
///
constexpr bool debug_trace_v =
#if defined(VULKIFY_DEBUG_TRACE)
	true;
#else
	false;
#endif
} // namespace vf
