#pragma once
#include <vulkify/graphics/primitives/shape.hpp>

namespace vf {
class Context;

///
/// \brief Data structure specifying a quad shape
///
struct QuadState {
	glm::vec2 size{100.0f, 100.0f};
	glm::vec2 origin{};
	QuadUV uv{};
};

///
/// \brief Primitive that models a quad / rectangle shape
///
class QuadShape : public Shape {
  public:
	using CreateInfo = QuadState;

	static constexpr auto name_v = "quad";

	QuadShape() = default;
	QuadShape(Context const& context, CreateInfo info = {}, std::string customName = {});

	QuadState const& state() const { return m_state; }
	glm::vec2 size() const { return m_state.size; }
	Texture const& texture() const { return m_texture; }

	QuadShape& setState(QuadState state);
	QuadShape& setTexture(Texture texture, bool resizeToMatch);

  protected:
	QuadShape& refresh();

	QuadState m_state{};
};
} // namespace vf