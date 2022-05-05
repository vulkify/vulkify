#include <detail/shared_impl.hpp>
#include <vulkify/context/context.hpp>

namespace vf {
Context::Context(UInstance&& instance) noexcept : m_instance(std::move(instance)) {}

auto Context::make(UInstance&& instance) -> Result {
	if (!instance) { return Error::eInvalidArgument; }
	return Context(std::move(instance));
}

Frame Context::frame(Rgba clear) {
	auto poll = m_instance->poll();
	auto surface = m_instance->beginPass(clear);
	return Frame{std::move(surface), poll, diffExchg(m_stamp)};
}
} // namespace vf
