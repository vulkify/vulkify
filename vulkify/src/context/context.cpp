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

Frame Context::nextFrame() {
	if (m_ready) { return {{}, diffExchg(m_stamp)}; }
	auto ret = Frame{m_instance->poll(), diffExchg(m_stamp)};
	m_ready = m_instance->beginPass();
	return ret;
}

bool Context::submit(Rgba clear) {
	if (m_ready) {
		auto const ret = m_instance->endPass(clear);
		m_ready = false;
		return ret;
	}
	return false;
}
} // namespace vf
