#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/instance/instance.hpp>
#include <vulkify/instance/instance_create_info.hpp>

namespace vf {
class VulkifyInstance : public Instance {
  public:
	using CreateInfo = InstanceCreateInfo;
	using Result = vf::Result<ktl::kunique_ptr<VulkifyInstance>>;

	static Result make(CreateInfo const& createInfo);

	~VulkifyInstance();

	Vram const& vram() const override;
	Gpu const& gpu() const override;
	bool closing() const override;
	glm::uvec2 framebufferExtent() const override;
	glm::uvec2 windowExtent() const override;
	glm::ivec2 position() const override;
	glm::vec2 contentScale() const override;
	CursorMode cursorMode() const override;
	glm::vec2 cursorPosition() const override;
	MonitorList monitors() const override;
	WindowFlags windowFlags() const override;
	AntiAliasing antiAliasing() const override;
	VSync vsync() const override;
	std::vector<Gpu> gpuList() const override;

	void show() override;
	void hide() override;
	void close() override;
	void setPosition(glm::ivec2 xy) override;
	void setExtent(glm::uvec2 size) override;
	void setIcons(std::span<Icon const> icons) override;
	void setCursorMode(CursorMode mode) override;
	Cursor makeCursor(Icon icon) override;
	void destroyCursor(Cursor cursor) override;
	bool setCursor(Cursor cursor) override;
	void setWindowed(glm::uvec2 extent) override;
	void setFullscreen(Monitor const& monitor, glm::uvec2 resolution) override;
	void updateWindowFlags(WindowFlags set, WindowFlags unset) override;
	bool setVSync(VSync vsync) override;
	Camera& camera() override;

	EventQueue poll() override;
	Surface beginPass(Rgba clear) override;
	bool endPass() override;

  private:
	struct Impl;
	ktl::kunique_ptr<Impl> m_impl;

	VulkifyInstance(ktl::kunique_ptr<Impl> impl) noexcept;
};
} // namespace vf
