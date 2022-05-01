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

	VulkifyInstance(VulkifyInstance&&) noexcept;
	VulkifyInstance& operator=(VulkifyInstance&&) noexcept;
	~VulkifyInstance() noexcept;

	Gpu const& gpu() const override;
	glm::ivec2 framebufferSize() const override;
	glm::ivec2 windowSize() const override;

	bool closing() const override;
	void show() override;
	void hide() override;
	void close() override;
	Poll poll() override;

	Surface beginPass() override;
	bool endPass() override;

	Vram const& vram() const override;

  private:
	struct Impl;
	ktl::kunique_ptr<Impl> m_impl;

	VulkifyInstance(ktl::kunique_ptr<Impl> impl) noexcept;
};

} // namespace vf
