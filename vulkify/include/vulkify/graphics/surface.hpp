#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/pipeline.hpp>
#include <concepts>
#include <iterator>
#include <span>

namespace vf {
struct RenderPass;
struct Pipeline;
struct Drawable;
struct Geometry;

///
/// \brief Surface being rendered to in a pass
///
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

	bool draw(Drawable const& drawable, Pipeline const& pipeline = {}) const;

  private:
	bool bind(Pipeline const& pipeline) const;
	bool draw(std::span<DrawModel const> models, Drawable const& drawable, Pipeline const& pipeline) const;

	ktl::fixed_pimpl<RenderPass, 256> m_renderPass;
};

// impl

template <std::output_iterator<DrawModel> Out>
void Surface::addDrawModels(std::span<DrawInstance const> instances, Out out) {
	for (auto const& instance : instances) { *out++ = instance.drawModel(); }
}
} // namespace vf
