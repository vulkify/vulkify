#pragma once
#include <vulkify/core/result.hpp>
#include <vulkify/graphics/detail/gfx_deferred.hpp>
#include <vulkify/graphics/geometry.hpp>
#include <vulkify/graphics/handle.hpp>

namespace vf {
struct GfxDevice;

///
/// \brief GPU Vertex (and index) buffer
///
class GeometryBuffer : public GfxDeferred {
  public:
	GeometryBuffer() = default;

	explicit GeometryBuffer(GfxDevice const& device);

	Result<void> write(Geometry geometry);

	Geometry geometry() const;

	Handle<GeometryBuffer> handle() const;
};
} // namespace vf
