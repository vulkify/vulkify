#pragma once
#include <vulkify/graphics/primitives/shape.hpp>

namespace vf {
///
/// \brief Data structure specifying a quad shape
///
struct CircleState {
	float diameter{100.0f};
	std::uint32_t points{64};
	glm::vec2 origin{};
	Rgba vertex{white_v};
};

///
/// \brief Primitive that models a quad / rectangle shape
///
class CircleShape : public Shape {
  public:
	using CreateInfo = CircleState;

	static constexpr auto name_v = "circle";

	CircleShape() = default;
	CircleShape(Context const& context, std::string name = name_v, CreateInfo info = {});

	CircleState const& state() const { return m_state; }
	float diameter() const { return m_state.diameter; }
	std::uint32_t points() const { return m_state.points; }
	Texture const& texture() const { return m_texture; }

	CircleShape& setState(CircleState state);
	CircleShape& setTexture(Texture texture, bool resizeToMatch);

  protected:
	CircleShape& refresh();

	CircleState m_state{};
};
} // namespace vf
