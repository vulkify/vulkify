#pragma once
#include <vulkify/core/result.hpp>
#include <vulkify/core/time.hpp>
#include <vulkify/instance/instance.hpp>

namespace vf {
class HeadlessInstance : public Instance {
  public:
	using Result = vf::Result<ktl::kunique_ptr<HeadlessInstance>>;

	static Result make(Time autoclose = 2s) { return ktl::make_unique<HeadlessInstance>(autoclose); }

	HeadlessInstance(Time autoclose);

	Vram const& vram() const override;
	Gpu const& gpu() const override { return m_gpu; }
	bool closing() const override { return Clock::now() - m_start > m_autoclose; }
	glm::uvec2 framebuffer_extent() const override { return m_framebuffer_extent; }
	glm::uvec2 window_extent() const override { return m_window_extent; }
	glm::ivec2 position() const override { return {}; }
	glm::vec2 content_scale() const override { return glm::vec2(1.0f); }
	CursorMode cursor_mode() const override { return {}; }
	glm::vec2 cursor_position() const override { return {}; }
	MonitorList monitors() const override { return {}; }
	WindowFlags window_flags() const override { return m_window_flags; }
	AntiAliasing anti_aliasing() const override { return AntiAliasing::eNone; }
	VSync vsync() const override { return {}; }
	std::vector<Gpu> gpu_list() const override { return {m_gpu}; }

	void show() override {}
	void hide() override {}
	void close() override {}
	void set_position(glm::ivec2) override {}
	void set_extent(glm::uvec2) override {}
	void set_cursor_mode(CursorMode) override {}
	Cursor make_cursor(Icon) override { return {}; }
	void destroy_cursor(Cursor) override {}
	bool set_cursor(Cursor) override { return false; }
	void set_icons(std::span<Icon const>) override {}
	void set_windowed(glm::uvec2) override {}
	void set_fullscreen(Monitor const&, glm::uvec2) override {}
	void update_window_flags(WindowFlags, WindowFlags) override {}
	Camera& camera() override { return m_camera; }

	EventQueue poll() override { return std::move(m_event_queue); }
	Surface begin_pass(Rgba) override { return {}; }
	bool end_pass() override { return true; }

	EventQueue m_event_queue{};
	glm::uvec2 m_framebuffer_extent{};
	glm::uvec2 m_window_extent{};
	WindowFlags m_window_flags{};
	Camera m_camera{};

  private:
	Gpu m_gpu = {"vulkify (headless)", {}, {}, {}, Gpu::Type::eOther};
	Clock::time_point m_start = Clock::now();
	Time m_autoclose{};
};
} // namespace vf
