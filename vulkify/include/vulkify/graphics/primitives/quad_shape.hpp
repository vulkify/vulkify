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
	explicit QuadShape(Context const& context, State initial = {});

	Rect bounds() const override { return {{size() * transform().scale, transform().position}}; }
	State const& state() const { return m_state; }
	glm::vec2 size() const { return m_state.size; }

	QuadShape& set_state(State state);
	QuadShape& set_texture(Ptr<refactor::Texture const> texture, bool resize_to_match = false);
	QuadShape& set_silhouette(float extrude, Rgba tint);

  protected:
	QuadShape& refresh();

	State m_state{};
};
} // namespace vf
