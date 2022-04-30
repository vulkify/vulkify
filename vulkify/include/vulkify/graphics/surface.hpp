#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/core/rgba.hpp>
#include <vulkify/graphics/pipeline.hpp>
#include <span>

namespace vf {
class GeometryBuffer;
struct RenderPass;
struct Pipeline;
struct DrawModel;
struct DrawInstance2;
class Texture;

class Surface {
  public:
	Surface() noexcept;
	Surface(RenderPass renderPass);
	Surface(Surface&&) noexcept;
	Surface& operator=(Surface) noexcept;
	~Surface();

	explicit operator bool() const;

	void setClear(Rgba rgba) const;
	bool bind(Pipeline const& pipeline = {}) const;
	bool draw(GeometryBuffer const& geometry, std::span<DrawModel const> models, Texture const* texture = {}) const;

  private:
	ktl::fixed_pimpl<RenderPass, 256> m_renderPass;
};
} // namespace vf
