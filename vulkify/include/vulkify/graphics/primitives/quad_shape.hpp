#pragma once
#include <vulkify/graphics/primitives/shape.hpp>

namespace vf {
///
/// \brief Primitive that models a quad / rectangle shape
///
class QuadShape : public Shape {
  public:
	using State = QuadCreateInfo;

	static constexpr auto name_v = "quad";

	QuadShape() = default;
	QuadShape(Context const& context, std::string name = name_v, State initial = {});

	State const& state() const { return m_state; }
	glm::vec2 size() const { return m_state.size; }
	Texture const& texture() const { return m_texture; }
	vf::Rect rect() const { return {{size() * transform().scale, transform().position}}; }

	QuadShape& setState(State state);
	QuadShape& setTexture(Texture texture, bool resizeToMatch);

  protected:
	QuadShape& refresh();

	State m_state{};
};
} // namespace vf
