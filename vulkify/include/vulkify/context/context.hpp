#pragma once
#include <vulkify/context/frame.hpp>
#include <vulkify/context/space.hpp>
#include <vulkify/core/result.hpp>
#include <optional>

namespace vf {
class Context;
using UContext = ktl::kunique_ptr<Context>;

class Context {
  public:
	using Result = vf::Result<UContext>;
	using Icon = Instance::Icon;

	static Result make(UInstance&& instance);

	Context& operator=(Context&&) = delete;
	~Context() noexcept;

	Instance const& instance() const { return *m_instance; }

	bool closing() const { return m_instance->closing(); }
	void show() { m_instance->show(); }
	void hide() { m_instance->hide(); }
	void close() { m_instance->close(); }

	Gpu const& gpu() const { return m_instance->gpu(); }
	glm::uvec2 framebufferSize() const { return m_instance->framebufferSize(); }
	glm::uvec2 windowSize() const { return m_instance->windowSize(); }
	glm::ivec2 position() const { return m_instance->position(); }
	glm::vec2 contentScale() const { return m_instance->contentScale(); }
	glm::vec2 cursorPosition() const { return m_instance->cursorPosition(); }
	CursorMode cursorMode() const { return m_instance->cursorMode(); }
	MonitorList monitors() const { return m_instance->monitors(); }
	WindowFlags windowFlags() const { return m_instance->windowFlags(); }
	View& view() const { return m_instance->view(); }
	Space space() const { return Space{m_instance->framebufferSize()}; }
	AntiAliasing antiAliasing() const { return m_instance->antiAliasing(); }

	Frame frame(Rgba clear = {});

	void setPosition(glm::ivec2 xy) { m_instance->setPosition(xy); }
	void setSize(glm::uvec2 size) { m_instance->setSize(size); }
	void setIcons(std::span<Icon const> icons) { m_instance->setIcons(icons); }
	void setCursorMode(CursorMode mode) { m_instance->setCursorMode(mode); }
	Cursor makeCursor(Icon icon) { return m_instance->makeCursor(icon); }
	void destroyCursor(Cursor cursor) { m_instance->destroyCursor(cursor); }
	bool setCursor(Cursor cursor) { return m_instance->setCursor(cursor); }
	void setWindowed(glm::uvec2 extent) { m_instance->setWindowed(extent); }
	void setFullscreen(Monitor const& monitor, glm::uvec2 resolution = {}) { m_instance->setFullscreen(monitor, resolution); }
	void updateWindowFlags(WindowFlags set, WindowFlags unset) { m_instance->updateWindowFlags(set, unset); }

	Vram const& vram() const { return m_instance->vram(); }

  private:
	Context(UInstance&& instance) noexcept;

	ktl::kunique_ptr<Instance> m_instance{};
	Clock::time_point m_stamp = now();
};
} // namespace vf
