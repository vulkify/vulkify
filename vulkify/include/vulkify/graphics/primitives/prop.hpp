#pragma once
#include <vulkify/graphics/drawable.hpp>
#include <vulkify/graphics/geometry_buffer.hpp>
#include <vulkify/graphics/primitive.hpp>

namespace vf {
///
/// \brief Abstract Primitive with protected GeometryBufer, TextureHandle, and DrawInstance
///
class Prop : public Primitive {
  public:
	Prop() = default;
	explicit Prop(Context const& context) : m_buffer(context) {}

	Transform const& transform() const { return m_instance.transform; }
	Transform& transform() { return m_instance.transform; }
	Rgba const& tint() const { return m_instance.tint; }
	Rgba& tint() { return m_instance.tint; }
	Handle<Texture> const& texture() const { return m_texture; }
	Handle<Texture>& texture() { return m_texture; }
	Geometry geometry() const { return m_buffer.geometry(); }
	GeometryBuffer const& buffer() const { return m_buffer; }

  protected:
	GeometryBuffer m_buffer{};
	Handle<Texture> m_texture{};
	DrawInstance m_instance{};
};
} // namespace vf
