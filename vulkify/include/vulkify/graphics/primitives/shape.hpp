#pragma once
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <vulkify/graphics/resources/geometry_buffer.hpp>
#include <vulkify/graphics/resources/texture.hpp>

namespace vf {
class Context;

///
/// \brief Base Primitive with protected GeometryBufer, TextureHandle, and DrawInstance
///
class Shape : public Primitive {
  public:
	Shape() = default;
	Shape(Context const& context, std::string name);

	Transform const& transform() const { return m_instance.transform; }
	Transform& transform() { return m_instance.transform; }
	Rgba const& tint() const { return m_instance.tint; }
	Rgba& tint() { return m_instance.tint; }
	GeometryBuffer const& geometry() const { return m_geometry; }
	TextureHandle const& texture() const { return m_texture; }

	void unset_silhouette() { m_silhouette.draw = false; }

	void draw(Surface const& surface, RenderState const& state = {}) const override;

  protected:
	GeometryBuffer m_geometry{};
	struct {
		GeometryBuffer gbo{};
		vf::Rgba tint{};
		bool draw{};
	} m_silhouette{};
	TextureHandle m_texture{};
	DrawInstance m_instance{};
};
} // namespace vf
