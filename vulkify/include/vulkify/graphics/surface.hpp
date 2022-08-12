#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/render_state.hpp>
#include <concepts>
#include <iterator>
#include <span>

namespace vf {
struct RenderPass;
struct RenderState;
struct Drawable;
struct Geometry;

///
/// \brief Surface being rendered to in a pass
///
class Surface {
  public:
	static constexpr std::size_t small_buffer_v = 8;

	template <std::output_iterator<DrawModel> Out>
	static void add_draw_models(std::span<DrawInstance const> instances, Out out);

	Surface() noexcept;
	Surface(RenderPass render_pass);
	Surface(Surface&&) noexcept;
	Surface& operator=(Surface&&) noexcept;
	~Surface();

	explicit operator bool() const;

	bool draw(Drawable const& drawable, RenderState const& state = {}) const;

  private:
	bool bind(RenderState const& state) const;
	bool draw(std::span<DrawModel const> models, Drawable const& drawable, RenderState const& state) const;

	ktl::fixed_pimpl<RenderPass, 256> m_render_pass;
};

// impl

template <std::output_iterator<DrawModel> Out>
void Surface::add_draw_models(std::span<DrawInstance const> instances, Out out) {
	for (auto const& instance : instances) { *out++ = instance.draw_model(); }
}
} // namespace vf

#include <ktl/fixed_pimpl.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/render_state.hpp>
#include <concepts>
#include <iterator>
#include <span>

namespace vf::refactor {
struct RenderPass;

///
/// \brief Surface being rendered to in a pass
///
class Surface {
  public:
	static constexpr std::size_t small_buffer_v = 8;

	template <std::output_iterator<DrawModel> Out>
	static void add_draw_models(std::span<DrawInstance const> instances, Out out);

	Surface() noexcept;
	Surface(RenderPass render_pass);
	Surface(Surface&&) noexcept;
	Surface& operator=(Surface&&) noexcept;
	~Surface();

	explicit operator bool() const;

	bool draw(Drawable const& drawable, RenderState const& state = {}) const;

  private:
	bool bind(RenderState const& state) const;
	bool draw(std::span<DrawModel const> models, Drawable const& drawable, RenderState const& state) const;

	ktl::fixed_pimpl<RenderPass, 256> m_render_pass;
};

// impl

template <std::output_iterator<DrawModel> Out>
void Surface::add_draw_models(std::span<DrawInstance const> instances, Out out) {
	for (auto const& instance : instances) { *out++ = instance.draw_model(); }
}
} // namespace vf::refactor
