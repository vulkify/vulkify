#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/render_state.hpp>
#include <concepts>
#include <iterator>
#include <span>

namespace vf {
struct RenderPass;

///
/// \brief Surface being rendered to in a pass
///
class Surface {
  public:
	static constexpr std::size_t small_buffer_v = 8;

	template <std::output_iterator<DrawModel> Out>
	static void add_draw_models(std::span<DrawInstance const> instances, Out out);

	Surface() = default;
	Surface(RenderPass const* render_pass) : m_render_pass(render_pass) { bind({}); }
	Surface(Surface&& rhs) noexcept : Surface() { swap(rhs); }
	Surface& operator=(Surface rhs) noexcept { return (swap(rhs), *this); }
	~Surface();

	explicit operator bool() const;

	bool draw(Drawable const& drawable, RenderState const& state = {}) const;

  private:
	void swap(Surface& rhs) noexcept { std::swap(m_render_pass, rhs.m_render_pass); }
	bool bind(RenderState const& state) const;
	bool draw(std::span<DrawModel const> models, Drawable const& drawable, RenderState const& state) const;

	RenderPass const* m_render_pass{};
};

// impl

template <std::output_iterator<DrawModel> Out>
void Surface::add_draw_models(std::span<DrawInstance const> instances, Out out) {
	for (auto const& instance : instances) { *out++ = instance.draw_model(); }
}
} // namespace vf
