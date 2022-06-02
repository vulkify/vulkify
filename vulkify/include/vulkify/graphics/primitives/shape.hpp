#pragma once
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <vulkify/graphics/resources/geometry_buffer.hpp>
#include <vulkify/graphics/resources/texture.hpp>

namespace vf {
class Context;

///
/// \brief Base Primitive with protected GeometryBufer, Texture, and DrawInstance
///
/// Note: The Texture member is not exposed to the public interface, but is referenecd in draw().
/// Derived types may use it as desired.
///
class Shape : public Primitive {
  public:
	struct Silhouette {
		float scale{};
		Rgba tint{white_v};

		void draw(Shape const& shape, Surface const& surface, RenderState const& state) const;
	};

	Shape() = default;
	Shape(Context const& context, std::string name);

	Transform const& transform() const { return m_instance.transform; }
	Transform& transform() { return m_instance.transform; }
	Rgba const& tint() const { return m_instance.tint; }
	Rgba& tint() { return m_instance.tint; }
	GeometryBuffer const& geometry() const { return m_geometry; }

	void draw(Surface const& surface, RenderState const& state = {}) const override;

	Silhouette silhouette{};

  protected:
	GeometryBuffer m_geometry{};
	Texture m_texture{};
	DrawInstance m_instance{};
};
} // namespace vf
