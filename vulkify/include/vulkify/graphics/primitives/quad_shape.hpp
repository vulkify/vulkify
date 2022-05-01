#pragma once
#include <vulkify/graphics/primitives/shape.hpp>

namespace vf {
struct QuadState {
	glm::vec2 size{100.0f, 100.0f};
	glm::vec2 origin{};
	Rgba rgba{white_v};
	QuadUV uv{};
};

class QuadShape : public Shape {
  public:
	using CreateInfo = QuadState;

	QuadShape() = default;
	QuadShape(Vram const& vram, std::string name, CreateInfo info = {});

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
