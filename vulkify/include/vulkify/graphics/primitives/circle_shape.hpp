#pragma once
#include <vulkify/graphics/primitives/shape.hpp>

namespace vf {
///
/// \brief Primitive that models a circle shape (as a regular polygon)
///
class CircleShape : public Shape {
  public:
	using State = PolygonCreateInfo;

	CircleShape() = default;
	explicit CircleShape(Context const& context, State initial = {});

	Rect bounds() const override { return {{0.5f * diameter() * transform().scale, transform().position}}; }
	State const& state() const { return m_state; }
	float diameter() const { return m_state.diameter; }
	std::uint32_t points() const { return m_state.points; }

	CircleShape& set_state(State state);
	CircleShape& set_texture(Ptr<Texture const> texture, bool resize_to_match = false);
	CircleShape& set_silhouette(float extrude, Rgba tint);

  protected:
	CircleShape& refresh();

	State m_state{};
};
} // namespace vf
