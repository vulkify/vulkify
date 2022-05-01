#pragma once
#include <vulkify/graphics/drawable.hpp>

namespace vf {
class Primitive {
  public:
	virtual ~Primitive() = default;

	virtual Drawable drawable() const = 0;
};
} // namespace vf
