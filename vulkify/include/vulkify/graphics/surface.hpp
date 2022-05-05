#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <concepts>
#include <iterator>
#include <span>

namespace vf {
struct RenderPass;
struct Pipeline;
struct Drawable;
struct Geometry;
struct PipelineState;
class Shader;

class Surface {
  public:
	static constexpr std::size_t small_buffer_v = 8;

	template <std::output_iterator<DrawModel> Out>
	static void addDrawModels(std::span<DrawInstance const> instances, Out out);

	Surface() noexcept;
	Surface(RenderPass renderPass);
	Surface(Surface&&) noexcept;
	Surface& operator=(Surface&&) noexcept;
	~Surface();

	explicit operator bool() const;

	bool setShader(Shader const& shader) const;
	bool draw(Drawable const& drawable) const;

  private:
	bool bind(PipelineState const& state) const;
	bool draw(std::span<DrawModel const> models, Drawable const& drawable) const;

	ktl::fixed_pimpl<RenderPass, 256> m_renderPass;
};

// impl

template <std::output_iterator<DrawModel> Out>
void Surface::addDrawModels(std::span<DrawInstance const> instances, Out out) {
	for (auto const& instance : instances) { *out++ = instance.drawModel(); }
}
} // namespace vf
