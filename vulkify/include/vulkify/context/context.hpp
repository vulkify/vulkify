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
	using Icon = Instance::Icon;

	static Result make(UInstance&& instance);

	Instance const& instance() const { return *m_instance; }

	bool closing() const { return m_instance->closing(); }
	void show() { m_instance->show(); }
	void hide() { m_instance->hide(); }
	void close() { m_instance->close(); }

	Gpu const& gpu() const { return m_instance->gpu(); }
	glm::uvec2 framebufferExtent() const { return m_instance->framebufferExtent(); }
	glm::uvec2 windowExtent() const { return m_instance->windowExtent(); }
	glm::ivec2 position() const { return m_instance->position(); }
	glm::vec2 contentScale() const { return m_instance->contentScale(); }
	glm::vec2 cursorPosition() const { return m_instance->cursorPosition() * contentScale(); }
	CursorMode cursorMode() const { return m_instance->cursorMode(); }
	MonitorList monitors() const { return m_instance->monitors(); }
	WindowFlags windowFlags() const { return m_instance->windowFlags(); }
	Camera& camera() const { return m_instance->camera(); }
	Rect area() const { return Rect{{m_instance->framebufferExtent()}}; }
	AntiAliasing antiAliasing() const { return m_instance->antiAliasing(); }
	float renderScale() const { return m_instance->renderScale(); }

	Frame frame(Rgba clear = {});

	void setPosition(glm::ivec2 xy) { m_instance->setPosition(xy); }
	void setExtent(glm::uvec2 size) { m_instance->setExtent(size); }
	void setIcons(std::span<Icon const> icons) { m_instance->setIcons(icons); }
	void setCursorMode(CursorMode mode) { m_instance->setCursorMode(mode); }
	Cursor makeCursor(Icon icon) { return m_instance->makeCursor(icon); }
	void destroyCursor(Cursor cursor) { m_instance->destroyCursor(cursor); }
	bool setCursor(Cursor cursor) { return m_instance->setCursor(cursor); }
	void setWindowed(glm::uvec2 extent) { m_instance->setWindowed(extent); }
	void setFullscreen(Monitor const& monitor, glm::uvec2 resolution = {}) { m_instance->setFullscreen(monitor, resolution); }
	void updateWindowFlags(WindowFlags set, WindowFlags unset) { m_instance->updateWindowFlags(set, unset); }
	void setRenderScale(float scale) { m_instance->setRenderScale(scale); }

	glm::mat3 unprojection() const;
	glm::vec2 unproject(glm::vec2 point) const { return unprojection() * glm::vec3(point, 1.0f); }

	Vram const& vram() const { return m_instance->vram(); }

  private:
	Context(UInstance&& instance) noexcept;

	UInstance m_instance{};
	Clock::time_point m_stamp = now();
};
} // namespace vf
