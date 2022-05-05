#pragma once
#include <ktl/async/kthread.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/instance/instance.hpp>
#include <atomic>

namespace vf {
class HeadlessInstance : public Instance {
  public:
	using Result = vf::Result<ktl::kunique_ptr<HeadlessInstance>>;

	static Result make() { return ktl::make_unique<HeadlessInstance>(); }

	HeadlessInstance();

	Vram const& vram() const override;
	Gpu const& gpu() const override { return m_gpu; }
	bool closing() const override { return m_closing; }
	glm::uvec2 framebufferSize() const override { return m_framebufferSize; }
	glm::uvec2 windowSize() const override { return m_windowSize; }
	glm::ivec2 position() const override { return {}; }
	glm::vec2 contentScale() const override { return glm::vec2(1.0f); }
	glm::vec2 cursorPosition() const override { return {}; }
	CursorMode cursorMode() const override { return {}; }
	MonitorList monitors() const override { return {}; }
	WindowFlags windowFlags() const override { return m_windowFlags; }
	View& view() const override { return m_view; }
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
	mutable View m_view{};
	std::atomic<bool> m_closing{};

  private:
	void run(ktl::kthread::stop_t stop);

	ktl::kthread m_thread{};
	Gpu m_gpu = {"vulkify (headless)", Gpu::Type::eOther};
};
} // namespace vf
