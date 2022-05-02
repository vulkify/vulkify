#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <concepts>
#include <iterator>
#include <span>

namespace vf {
struct RenderPass;
struct Pipeline;
struct Drawable;

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

	void setClear(Rgba rgba) const;
	bool bind(std::string shaderPath) const;
	bool draw(Drawable const& drawable) const;
	bool draw(Primitive const& primitive) const { return draw(primitive.drawable()); }

  private:
	bool draw(std::span<DrawModel const> models, Drawable const& drawable) const;

	ktl::fixed_pimpl<RenderPass, 256> m_renderPass;
};

// impl

template <std::output_iterator<DrawModel> Out>
void Surface::addDrawModels(std::span<DrawInstance const> instances, Out out) {
	for (auto const& instance : instances) { *out++ = instance.drawModel(); }
}
} // namespace vf
