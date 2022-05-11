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
	glm::uvec2 framebufferSize() const override { return m_framebufferSize; }
	glm::uvec2 windowSize() const override { return m_windowSize; }
	glm::ivec2 position() const override { return {}; }
	glm::vec2 contentScale() const override { return glm::vec2(1.0f); }
	CursorMode cursorMode() const override { return {}; }
	glm::vec2 cursorPosition() const override { return {}; }
	MonitorList monitors() const override { return {}; }
	WindowFlags windowFlags() const override { return m_windowFlags; }
	RenderView& view() const override { return m_view; }
	AntiAliasing antiAliasing() const override { return AntiAliasing::eNone; }

	void show() override {}
	void hide() override {}
	void close() override {}
	void setPosition(glm::ivec2) override {}
	void setSize(glm::uvec2) override {}
	void setCursorMode(CursorMode) override {}
	Cursor makeCursor(Icon) override { return {}; }
	void destroyCursor(Cursor) override {}
	bool setCursor(Cursor) override { return false; }
	void setIcons(std::span<Icon const>) override {}
	void setWindowed(glm::uvec2) override {}
	void setFullscreen(Monitor const&, glm::uvec2) override {}
	void updateWindowFlags(WindowFlags, WindowFlags) override {}

	Poll poll() override { return std::move(m_poll); }
	Surface beginPass(Rgba) override { return {}; }
	bool endPass() override { return true; }

	Poll m_poll{};
	glm::uvec2 m_framebufferSize{};
	glm::uvec2 m_windowSize{};
	WindowFlags m_windowFlags{};
	mutable RenderView m_view{};

  private:
	Gpu m_gpu = {"vulkify (headless)", {}, Gpu::Type::eOther};
	Clock::time_point m_start = Clock::now();
	Time m_autoclose{};
};
} // namespace vf
