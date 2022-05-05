#pragma once

namespace vf {
class Surface;

///
/// \brief Base class for drawable/transformable object on screen
///
class Primitive {
  public:
	virtual ~Primitive() = default;

	virtual void draw(Surface const& surface) const = 0;
};
} // namespace vf
