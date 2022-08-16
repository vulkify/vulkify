#include <detail/gfx_device.hpp>
#include <detail/render_pass.hpp>
#include <vulkify/context/context.hpp>
#include <vulkify/context/frame.hpp>
#include <vulkify/instance/vf_instance.hpp>

namespace vf {
namespace {
struct {
	Camera camera{};
} g_dummy{};
} // namespace

Camera Frame::camera() const { return m_surface.m_render_pass && m_surface.m_render_pass->cam.camera ? *m_surface.m_render_pass->cam.camera : g_dummy.camera; }

void Frame::set_camera(Camera const& cam) const {
	if (m_surface.m_render_pass && m_surface.m_render_pass->cam.camera) { *m_surface.m_render_pass->cam.camera = cam; }
}

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

glm::mat3 Context::unprojection() const {
	auto const& camera = m_instance->camera();
	auto const scale = glm::vec2(1.0f / camera.view.get_scale(framebuffer_extent()));
	return Transform::scaling(scale) * Transform::rotation(camera.orientation) * Transform::translation(camera.position);
}
} // namespace vf
