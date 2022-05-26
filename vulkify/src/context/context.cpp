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

glm::mat3 Context::unprojection() const {
	auto const& camera = m_instance->camera();
	auto const scale = glm::vec2(m_instance->renderScale() * 1.0f / camera.view.getScale(framebufferExtent()));
	return Transform::scaling(scale) * Transform::rotation(camera.orientation) * Transform::translation(camera.position);
}
} // namespace vf
