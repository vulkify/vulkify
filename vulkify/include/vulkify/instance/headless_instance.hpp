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

	Gpu const& gpu() const override { return m_gpu; }
	glm::ivec2 framebufferSize() const override { return m_framebufferSize; }
	glm::ivec2 windowSize() const override { return m_windowSize; }

	bool closing() const override { return m_closing; }
	void show() override {}
	void hide() override {}
	void close() override {}
	Poll poll() override { return std::move(m_poll); }

	Surface beginPass() override { return {}; }
	bool endPass() override { return true; }

	Vram const& vram() const override;

	Poll m_poll{};
	glm::ivec2 m_framebufferSize{};
	glm::ivec2 m_windowSize{};
	std::atomic<bool> m_closing{};

  private:
	void run(ktl::kthread::stop_t stop);

	ktl::kthread m_thread{};
	Gpu m_gpu = {"vulkify (headless)", Gpu::Type::eOther};
};
} // namespace vf
