#include <detail/shared_impl.hpp>
#include <vulkify/context/context.hpp>

#include <vulkify/instance/vf_instance.hpp>

namespace vf {
Context::Context(UInstance&& instance) noexcept : m_instance(std::move(instance)) {}

auto Context::make(UInstance&& instance) -> Result {
	if (!instance) { return Error::eInvalidArgument; }
	return Context(std::move(instance));
}

Frame Context::frame(Rgba clear) {
	auto poll = m_instance->poll();
	auto surface = m_instance->begin_pass(clear);
	return Frame{std::move(surface), poll, diff_exchg(m_stamp)};
}

refactor::Frame Context::frame2(Rgba clear) {
	auto poll = m_instance->poll();
	auto surface = dynamic_cast<refactor::VulkifyInstance&>(*m_instance).begin_pass2(clear);
	return refactor::Frame{std::move(surface), poll, diff_exchg(m_stamp)};
}

glm::mat3 Context::unprojection() const {
	auto const& camera = m_instance->camera();
	auto const scale = glm::vec2(1.0f / camera.view.get_scale(framebuffer_extent()));
	return Transform::scaling(scale) * Transform::rotation(camera.orientation) * Transform::translation(camera.position);
}
} // namespace vf
