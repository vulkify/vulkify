#pragma once
#include <vulkify/graphics/drawable.hpp>

namespace vf {
///
/// \brief Base class for drawable/transformable object on screen
///
class Primitive {
  public:
	virtual ~Primitive() = default;

	virtual Drawable drawable() const = 0;
};
} // namespace vf
