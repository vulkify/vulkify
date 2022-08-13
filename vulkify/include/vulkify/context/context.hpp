#pragma once
#include <vulkify/context/frame.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/instance/instance.hpp>

namespace vf {
///
/// \brief Vulkify "HQ": owns Instance and drives rendering frames
///
class Context {
  public:
	using Result = vf::Result<Context>;

	static Result make(UInstance&& instance);

	Instance const& instance() const { return *m_instance; }

	bool closing() const { return m_instance->closing(); }
	void show() { m_instance->show(); }
	void hide() { m_instance->hide(); }
	void close() { m_instance->close(); }

	Gpu const& gpu() const { return m_instance->gpu(); }
	glm::uvec2 framebuffer_extent() const { return m_instance->framebuffer_extent(); }
	glm::uvec2 window_extent() const { return m_instance->window_extent(); }
	glm::ivec2 position() const { return m_instance->position(); }
	glm::vec2 content_scale() const { return m_instance->content_scale(); }
	glm::vec2 cursor_position() const { return m_instance->cursor_position() * content_scale(); }
	CursorMode cursor_mode() const { return m_instance->cursor_mode(); }
	MonitorList monitors() const { return m_instance->monitors(); }
	WindowFlags window_flags() const { return m_instance->window_flags(); }
	Camera& camera() const { return m_instance->camera(); }
	Rect area() const { return Rect{{m_instance->framebuffer_extent()}}; }
	AntiAliasing anti_aliasing() const { return m_instance->anti_aliasing(); }
	VSync vsync() const { return m_instance->vsync(); }

	Frame frame_old(Rgba clear = {});
	// TODO: fixup
	refactor::Frame frame(Rgba clear = {});

	void set_position(glm::ivec2 xy) { m_instance->set_position(xy); }
	void set_extent(glm::uvec2 size) { m_instance->set_extent(size); }
	void set_icons(std::span<Icon const> icons) { m_instance->set_icons(icons); }
	void set_cursor_mode(CursorMode mode) { m_instance->set_cursor_mode(mode); }
	Cursor make_cursor(Icon icon) { return m_instance->make_cursor(icon); }
	void destroy_cursor(Cursor cursor) { m_instance->destroy_cursor(cursor); }
	bool set_cursor(Cursor cursor) { return m_instance->set_cursor(cursor); }
	void set_windowed(glm::uvec2 extent) { m_instance->set_windowed(extent); }
	void set_fullscreen(Monitor const& monitor, glm::uvec2 resolution = {}) { m_instance->set_fullscreen(monitor, resolution); }
	void update_window_flags(WindowFlags set, WindowFlags unset) { m_instance->update_window_flags(set, unset); }

	glm::mat3 unprojection() const;
	glm::vec2 unproject(glm::vec2 point) const { return unprojection() * glm::vec3(point, 1.0f); }

	refactor::GfxDevice const& device() const { return m_instance->gfx_device(); }
	Vram const& vram() const { return m_instance->vram(); }

  private:
	Context(UInstance&& instance) noexcept;

	UInstance m_instance{};
	Clock::time_point m_stamp = now();
};
} // namespace vf
