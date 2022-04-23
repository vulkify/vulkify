#pragma once
#include <glm/vec2.hpp>
#include <ktl/enum_flags/enum_flags.hpp>
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/instance/instance.hpp>
#include <string>

namespace vf {
class VulkifyInstance : public Instance {
  public:
	enum class Flag { eBorderless, eNoResize, eHidden, eMaximized };
	using Flags = ktl::enum_flags<Flag, std::uint8_t>;

	struct Info;
	using Result = vf::Result<ktl::kunique_ptr<VulkifyInstance>>;

	static Result make(Info const& info);

	VulkifyInstance(VulkifyInstance&&) noexcept;
	VulkifyInstance& operator=(VulkifyInstance&&) noexcept;
	~VulkifyInstance() noexcept;

	GPU const& gpu() const override;
	glm::ivec2 framebufferSize() const override;
	glm::ivec2 windowSize() const override;

	bool closing() const override;
	void show() override;
	void hide() override;
	void close() override;
	Poll poll() override;

	bool beginPass() override;
	bool endPass() override;

  private:
	struct Impl;
	ktl::kunique_ptr<Impl> m_impl;

	VulkifyInstance(ktl::kunique_ptr<Impl> impl) noexcept;
};

struct VulkifyInstance::Info {
	std::string title{"(Untitled)"};
	glm::uvec2 extent{1280, 720};
	Flags flags{};
};
} // namespace vf
