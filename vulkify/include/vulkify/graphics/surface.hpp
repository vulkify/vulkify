#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/graphics/draw_params.hpp>
#include <vulkify/graphics/pipeline_state.hpp>

namespace vf {
class GeometryBuffer;
struct RenderPass;
struct PipelineState;
struct DrawParams;

class Surface {
  public:
	Surface() noexcept;
	Surface(RenderPass renderPass);
	Surface(Surface&&) noexcept;
	Surface& operator=(Surface) noexcept;
	~Surface();

	explicit operator bool() const;

	void setClear(Rgba rgba) const;
	bool bind(PipelineState const& pipeline = {}) const;
	bool draw(GeometryBuffer const& geometry, DrawParams const& params = {}) const;

  private:
	ktl::fixed_pimpl<RenderPass, 128> m_renderPass;
};
} // namespace vf
