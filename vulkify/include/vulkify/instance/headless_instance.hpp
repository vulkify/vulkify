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

	void show() const override {}
	void hide() const override {}
	void close() const override {}
	void setPosition(glm::ivec2) const override {}
	void setSize(glm::uvec2) const override {}
	void setCursorMode(CursorMode) const override {}
	Cursor makeCursor(Icon) const override { return {}; }
	void destroyCursor(Cursor) const override {}
	bool setCursor(Cursor) const override { return false; }
	void setIcons(std::span<Icon const>) const override {}
	void setWindowed(glm::uvec2) const override {}
	void setFullscreen(Monitor const&, glm::uvec2) const override {}
	void updateWindowFlags(WindowFlags, WindowFlags) const override {}

	Poll poll() override { return std::move(m_poll); }
	Surface beginPass() override { return {}; }
	bool endPass() override { return true; }

	Poll m_poll{};
	glm::uvec2 m_framebufferSize{};
	glm::uvec2 m_windowSize{};
	WindowFlags m_windowFlags{};
	std::atomic<bool> m_closing{};

  private:
	void run(ktl::kthread::stop_t stop);

	ktl::kthread m_thread{};
	Gpu m_gpu = {"vulkify (headless)", Gpu::Type::eOther};
};
} // namespace vf
