#pragma once
#include <vulkify/core/result.hpp>
#include <vulkify/graphics/detail/resource.hpp>
#include <vulkify/graphics/geometry.hpp>

namespace vf {
class Context;

///
/// \brief GPU Vertex (and index) buffer
///
class GeometryBuffer : public GfxResource {
  public:
	struct Counts {
		std::uint32_t vertices{};
		std::uint32_t indices{};
	};

	GeometryBuffer() = default;
	explicit GeometryBuffer(Context const& context);

	Result<void> write(Geometry geometry);

	Geometry geometry() const;
	Counts counts() const { return m_counts; }

  private:
	Counts m_counts{};
};
} // namespace vf
