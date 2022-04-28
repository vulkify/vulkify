#pragma once
#include <vulkify/core/transform.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/draw_model.hpp>

namespace vf {
class Surface;

struct DrawInstance {
	Transform transform{};
	Rgba tint{};

	DrawModel model() const;
};

class Drawable {
  public:
	Drawable() = default;
	Drawable(Vram const& vram, std::string name);
	Drawable(Drawable&&) = default;
	Drawable& operator=(Drawable&&) = default;
	virtual ~Drawable() = default;

	explicit operator bool() const { return static_cast<bool>(m_geometry); }

	Transform& transform() { return m_instance.transform; }
	Transform const& transform() const { return m_instance.transform; }
	Rgba& tint() { return m_instance.tint; }
	Rgba const& tint() const { return m_instance.tint; }
	Geometry const& geometry() const { return m_geometry.geometry(); }
	bool setGeometry(Geometry geometry);

	void draw(Surface const& surface) const;

  protected:
	GeometryBuffer m_geometry{};
	DrawInstance m_instance{};
};
} // namespace vf
