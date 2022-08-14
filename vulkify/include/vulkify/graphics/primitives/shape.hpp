#pragma once
#include <vulkify/graphics/primitives/prop.hpp>

namespace vf {
class Texture;

///
/// \brief Base Primitive for shapes
///
class Shape : public Prop {
  public:
	Shape() = default;

	explicit Shape(GfxDevice const& device);

	virtual Rect bounds() const = 0;
	void unset_silhouette() { m_silhouette.draw = false; }

	void draw(Surface const& surface, RenderState const& state = {}) const override;

  protected:
	struct {
		GeometryBuffer buffer{};
		Rgba tint{};
		bool draw{};
	} m_silhouette{};
};
} // namespace vf
