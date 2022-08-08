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
	explicit Shape(Context const& context);

	void unset_silhouette() { m_silhouette.draw = false; }

	void draw(Surface const& surface, RenderState const& state = {}) const override;

  protected:
	struct {
		GeometryBuffer buffer{};
		vf::Rgba tint{};
		bool draw{};
	} m_silhouette{};
};
} // namespace vf
