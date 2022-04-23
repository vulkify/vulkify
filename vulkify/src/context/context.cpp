#include <vulkify/context/context.hpp>

namespace vf {
namespace {
Context* g_active{};
}

Context::Context(UInstance&& instance) noexcept : m_instance(std::move(instance)) { g_active = this; }
Context::~Context() noexcept { g_active = {}; }

vf::Result<ktl::kunique_ptr<Context>> Context::make(Info, UInstance&& instance) {
	if (g_active) { return Error::eDuplicateInstance; }
	return ktl::kunique_ptr<Context>(new Context(std::move(instance)));
}
} // namespace vf
