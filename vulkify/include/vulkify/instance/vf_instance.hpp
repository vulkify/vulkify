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

	GfxDevice const& gfx_device() const override;
	Gpu const& gpu() const override;
	bool closing() const override;
	glm::uvec2 framebuffer_extent() const override;
	glm::uvec2 window_extent() const override;
	glm::ivec2 position() const override;
	glm::vec2 content_scale() const override;
	CursorMode cursor_mode() const override;
	glm::vec2 cursor_position() const override;
	MonitorList monitors() const override;
	WindowFlags window_flags() const override;
	AntiAliasing anti_aliasing() const override;
	VSync vsync() const override;
	std::vector<Gpu> gpu_list() const override;
	ZOrder default_z_order() const override;

	void show() override;
	void hide() override;
	void close() override;
	void set_position(glm::ivec2 xy) override;
	void set_extent(glm::uvec2 size) override;
	void set_icons(std::span<Icon const> icons) override;
	void set_cursor_mode(CursorMode mode) override;
	Cursor make_cursor(Icon icon) override;
	void destroy_cursor(Cursor cursor) override;
	bool set_cursor(Cursor cursor) override;
	void set_windowed(glm::uvec2 extent) override;
	void set_fullscreen(Monitor const& monitor, glm::uvec2 resolution) override;
	void update_window_flags(WindowFlags set, WindowFlags unset) override;
	Camera& camera() override;

	EventQueue poll() override;
	Surface begin_pass(Rgba clear) override;
	bool end_pass() override;

  private:
	struct Impl;
	ktl::kunique_ptr<Impl> m_impl;

	VulkifyInstance(ktl::kunique_ptr<Impl> impl) noexcept;
};
} // namespace vf
