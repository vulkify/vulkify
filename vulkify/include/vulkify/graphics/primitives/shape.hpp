#pragma once
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/geometry_buffer.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
class Context;

///
/// \brief Base Primitive with protected GeometryBufer, TextureHandle, and DrawInstance
///
class Shape : public Primitive {
  public:
	Shape() = default;
	explicit Shape(Context const& context);

	Transform const& transform() const { return m_instance.transform; }
	Transform& transform() { return m_instance.transform; }
	Rgba const& tint() const { return m_instance.tint; }
	Rgba& tint() { return m_instance.tint; }
	TextureHandle const& texture() const { return m_texture; }
	TextureHandle& texture() { return m_texture; }
	Geometry geometry() const { return m_buffer.geometry(); }
	GeometryBuffer const& buffer() const { return m_buffer; }

	void unset_silhouette() { m_silhouette.draw = false; }

	void draw(Surface const& surface, RenderState const& state = {}) const override;

  protected:
	GeometryBuffer m_buffer{};
	struct {
		GeometryBuffer buffer{};
		vf::Rgba tint{};
		bool draw{};
	} m_silhouette{};
	TextureHandle m_texture{};
	DrawInstance m_instance{};
};
} // namespace vf
