#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/graphics/bitmap.hpp>
#include <vulkify/graphics/camera.hpp>
#include <vulkify/graphics/surface.hpp>
#include <vulkify/instance/cursor.hpp>
#include <vulkify/instance/event.hpp>
#include <vulkify/instance/event_queue.hpp>
#include <vulkify/instance/gpu.hpp>
#include <vulkify/instance/icon.hpp>
#include <vulkify/instance/instance_enums.hpp>
#include <vulkify/instance/monitor.hpp>
#include <span>

namespace vf {
struct GfxDevice;

class Instance {
  public:
	static constexpr std::size_t max_events_v = 16;
	static constexpr std::size_t max_scancodes_v = 16;

	virtual ~Instance() = default;

	virtual GfxDevice const& gfx_device() const = 0;
	virtual Gpu const& gpu() const = 0;
	virtual bool closing() const = 0;
	virtual glm::uvec2 framebuffer_extent() const = 0;
	virtual glm::uvec2 window_extent() const = 0;
	virtual glm::ivec2 position() const = 0;
	virtual glm::vec2 content_scale() const = 0;
	virtual CursorMode cursor_mode() const = 0;
	virtual glm::vec2 cursor_position() const = 0;
	virtual MonitorList monitors() const = 0;
	virtual WindowFlags window_flags() const = 0;
	virtual AntiAliasing anti_aliasing() const = 0;
	virtual VSync vsync() const = 0;
	virtual std::vector<Gpu> gpu_list() const = 0;
	virtual ZOrder default_z_order() const = 0;

	virtual void show() = 0;
	virtual void hide() = 0;
	virtual void close() = 0;
	virtual void set_position(glm::ivec2 xy) = 0;
	virtual void set_extent(glm::uvec2 extent) = 0;
	virtual void set_icons(std::span<Icon const> icons) = 0;
	virtual void set_cursor_mode(CursorMode mode) = 0;
	virtual Cursor make_cursor(Icon icon) = 0;
	virtual void destroy_cursor(Cursor cursor) = 0;
	virtual bool set_cursor(Cursor cursor) = 0;
	virtual void set_windowed(glm::uvec2 extent) = 0;
	virtual void set_fullscreen(Monitor const& monitor, glm::uvec2 resolution = {}) = 0;
	virtual void update_window_flags(WindowFlags set, WindowFlags unset = {}) = 0;
	virtual Camera& camera() = 0;
	virtual void lock_aspect_ratio(bool lock) = 0;

	virtual EventQueue poll() = 0;
	virtual Surface begin_pass(Rgba clear) = 0;
	virtual bool end_pass() = 0;
};

using UInstance = ktl::kunique_ptr<Instance>;
} // namespace vf
