#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/core/rgba.hpp>
#include <vulkify/pipeline/spec.hpp>

namespace vf {
class Canvas {
  public:
	struct Impl;

	Canvas() noexcept;
	Canvas(Impl impl);
	Canvas(Canvas&&) noexcept;
	Canvas& operator=(Canvas) noexcept;
	~Canvas() noexcept;

	explicit operator bool() const;

	void setClear(Rgba rgba) const;
	bool bind(PipelineSpec const& pipeline) const;
	void draw(std::size_t vertexCount, std::size_t indexCount) const;

  private:
	ktl::fixed_pimpl<Impl, 64> m_impl;
	friend class Context;
};
} // namespace vf
