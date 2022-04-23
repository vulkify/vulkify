#pragma once
#include <vulkify/core/result.hpp>
#include <vulkify/instance/instance.hpp>

namespace vf {
class HeadlessInstance : public Instance {
  public:
	using Result = vf::Result<ktl::kunique_ptr<HeadlessInstance>>;

	static Result make() { return ktl::make_unique<HeadlessInstance>(); }

	GPU const& gpu() const override { return m_gpu; }

	bool closing() const override { return m_closing; }
	void show() override {}
	void hide() override {}
	void close() override {}
	Poll poll() override { return std::move(m_poll); }

	Poll m_poll{};
	bool m_closing{};

  private:
	GPU m_gpu = {"vulkify (headless)", GPU::Type::eOther};
};
} // namespace vf
