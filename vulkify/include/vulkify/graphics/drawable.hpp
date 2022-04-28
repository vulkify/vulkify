#pragma once
#include <vulkify/core/transform.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/draw_instance_data.hpp>

namespace vf {
class Surface;

class Drawable {
  public:
	using InstanceData = DrawInstanceData;

	Drawable() = default;
	Drawable(Vram const& vram, std::string name);
	Drawable(Drawable&&) = default;
	Drawable& operator=(Drawable&&) = default;
	virtual ~Drawable() = default;

	explicit operator bool() const { return static_cast<bool>(m_geometry); }

	Transform& transform() { return m_transform; }
	Transform const& transform() const { return m_transform; }
	Rgba& tint() { return m_tint; }
	Rgba const& tint() const { return m_tint; }
	Geometry const& geometry() const { return m_geometry.geometry(); }
	bool setGeometry(Geometry geometry);

	void draw(Surface const& surface) const;

  protected:
	GeometryBuffer m_geometry{};
	Transform m_transform{};
	Rgba m_tint{};
};
} // namespace vf
