#pragma once
#include <ktl/fixed_any.hpp>

namespace vf {
namespace detail {
template <std::size_t Count>
using Handle = ktl::fixed_any<Count * sizeof(void*)>;
}

struct TextureHandle {
	detail::Handle<2> handle{};

	explicit operator bool() const { return !handle.empty(); }
};

struct ShaderHandle {
	detail::Handle<1> handle{};

	explicit operator bool() const { return !handle.empty(); }
};
} // namespace vf
