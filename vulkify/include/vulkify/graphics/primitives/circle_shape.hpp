#pragma once
#include <vulkify/graphics/primitives/shape.hpp>

namespace vf {
///
/// \brief Primitive that models a circle shape (as a regular polygon)
///
class CircleShape : public Shape {
  public:
	using State = PolygonCreateInfo;

	static constexpr auto name_v = "circle";

	CircleShape() = default;
	CircleShape(Context const& context, std::string name = name_v, State initial = {});

	State const& state() const { return m_state; }
	float diameter() const { return m_state.diameter; }
	std::uint32_t points() const { return m_state.points; }
	Texture const& texture() const { return m_texture; }

	CircleShape& set_state(State state);
	CircleShape& set_texture(Texture texture, bool resizeToMatch);

  protected:
	CircleShape& refresh();

	State m_state{};
};
} // namespace vf
