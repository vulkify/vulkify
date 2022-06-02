#pragma once
#include <vulkify/graphics/render_state.hpp>

namespace vf {
class Surface;

///
/// \brief Base class for drawable/transformable object on screen
///
class Primitive {
  public:
	virtual ~Primitive() = default;

	virtual void draw(Surface const& surface, RenderState const& state = {}) const = 0;
};
} // namespace vf
