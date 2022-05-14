#pragma once
#include <ktl/not_null.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/primitive.hpp>

namespace vf {
class Ttf;

class Text : public Primitive {
  public:
	using Pivot = glm::vec2;

	Text() = default;
	Text(Context const& context, std::string name);

	explicit operator bool() const;

	Text& setFont(ktl::not_null<Ttf*> ttf);

	Rgba& tint() { return m_instance.tint; }
	Rgba const& tint() const { return m_instance.tint; }
	Transform& transform() { return m_instance.transform; }
	Transform const& transform() const { return m_instance.transform; }

	void draw(Surface const& surface) const override;

	std::string text{};
	Pivot pivot{};

  protected:
	Drawable drawable() const;
	void update() const;

  private:
	mutable GeometryBuffer m_gbo{};
	DrawInstance m_instance{};
	Ttf* m_ttf{};
};
} // namespace vf
