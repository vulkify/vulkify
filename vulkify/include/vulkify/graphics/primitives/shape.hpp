#pragma once
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
class Context;

///
/// \brief Base Primitive with protected GeometryBufer, Texture, and DrawInstance
///
/// Note: The Texture member is not exposed to the public interface, but is referenecd in the returned Drawable.
/// Derived types may use it as desired.
///
class Shape : public Primitive {
  public:
	Shape() = default;
	Shape(Context const& context, std::string name);

	Transform const& transform() const { return m_instance.transform; }
	Transform& transform() { return m_instance.transform; }
	Rgba const& tint() const { return m_instance.tint; }
	Rgba& tint() { return m_instance.tint; }

	void draw(Surface const& surface) const override;

  protected:
	GeometryBuffer m_geometry{};
	Texture m_texture{};
	DrawInstance m_instance{};
};

class OutlinedShape : public Shape {
  public:
	OutlinedShape() = default;
	OutlinedShape(Context const& context, std::string name);

	float outlineWidth() const { return m_outline.state.lineWidth; }
	Rgba outlineRgba() const { return m_outlineRgba; }
	void setOutline(float lineWidth, Rgba rgba);

	void draw(Surface const& surface) const override;

  protected:
	virtual void refreshOutline();
	void writeOutline(Geometry geometry);

	GeometryBuffer m_outline{};
	Rgba m_outlineRgba{};
};
} // namespace vf
