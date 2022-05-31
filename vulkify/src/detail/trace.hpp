#pragma once

#if defined(VULKIFY_DEBUG_TRACE)
#include <ktl/kformat.hpp>
#endif

namespace vf::trace {
enum class Type { eError, eWarn, eInfo };

struct Payload {
	std::string message{};
	std::string_view prefix{};
	Type type{};
};

void log(Payload);
} // namespace vf::trace

#if defined(VULKIFY_DEBUG_TRACE)
#define VF_TRACE(prefix, type, msg) ::vf::trace::log({msg, prefix, type})
#define VF_TRACEF(prefix, type, fmt, ...) ::vf::trace::log({::ktl::kformat(fmt, __VA_ARGS__), prefix, type})
#define VF_TRACEE(prefix, fmt, ...) VF_TRACEF(prefix, ::vf::trace::Type::eError, fmt, __VA_ARGS__)
#define VF_TRACEW(prefix, fmt, ...) VF_TRACEF(prefix, ::vf::trace::Type::eWarn, fmt, __VA_ARGS__)
#define VF_TRACEI(prefix, fmt, ...) VF_TRACEF(prefix, ::vf::trace::Type::eInfo, fmt, __VA_ARGS__)

#else
#define VF_TRACE(prefix, type, msg)
#define VF_TRACEF(prefix, type, fmt, ...)
#define VF_TRACEE(prefix, fmt, ...)
#define VF_TRACEW(prefix, fmt, ...)
#define VF_TRACEI(prefix, fmt, ...)
#endif
