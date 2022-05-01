#pragma once
#include <vulkify/graphics/drawable.hpp>

namespace vf {
struct Pipeline;
class Surface;

class Primitive {
  public:
	virtual ~Primitive() = default;

	virtual Drawable drawable() const = 0;

	void draw(Surface const& surface) const;
};
} // namespace vf
