#include <detail/shared_impl.hpp>
#include <vulkify/context/context.hpp>

namespace vf {
namespace {
Context* g_active{};
}

Context::Context(UInstance&& instance) noexcept : m_instance(std::move(instance)) { g_active = this; }
Context::~Context() noexcept { g_active = {}; }

vf::Result<ktl::kunique_ptr<Context>> Context::make(UInstance&& instance) {
	if (g_active) { return Error::eDuplicateInstance; }
	if (!instance) { return Error::eInvalidArgument; }
	return ktl::kunique_ptr<Context>(new Context(std::move(instance)));
}

Frame Context::frame() {
	auto poll = m_instance->poll();
	auto surface = m_instance->beginPass();
	return Frame{std::move(surface), poll, diffExchg(m_stamp)};
}
} // namespace vf
