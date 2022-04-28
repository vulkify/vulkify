#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/core/rgba.hpp>
#include <vulkify/graphics/pipeline_state.hpp>
#include <span>

namespace vf {
class GeometryBuffer;
struct RenderPass;
struct PipelineState;
struct DrawInstanceData;

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
	bool draw(GeometryBuffer const& geometry, char const* name, std::span<DrawInstanceData const> instances) const;

  private:
	ktl::fixed_pimpl<RenderPass, 128> m_renderPass;
};
} // namespace vf
