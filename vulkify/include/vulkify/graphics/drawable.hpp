#pragma once
#include <vulkify/core/transform.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/draw_params.hpp>

namespace vf {
class Surface;

struct Drawer {
	Transform transform{};
	mutable DrawParams drawParams{};

	void operator()(Surface const& surface, GeometryBuffer const& geometry) const;
};

class Drawable {
  public:
	using Params = DrawParams;

	Drawable() = default;
	Drawable(Vram const& vram, std::string name);
	Drawable(Drawable&&) = default;
	Drawable& operator=(Drawable&&) = default;
	virtual ~Drawable() = default;

	explicit operator bool() const { return static_cast<bool>(m_geometry); }

	Transform& transform() { return m_drawer.transform; }
	Transform const& transform() const { return m_drawer.transform; }
	Params& params() { return m_drawer.drawParams; }
	Params const& params() const { return m_drawer.drawParams; }
	Geometry const& geometry() const { return m_geometry.geometry(); }
	bool setGeometry(Geometry geometry);

	void draw(Surface const& surface) const;

  protected:
	GeometryBuffer m_geometry{};
	Drawer m_drawer{};
};
} // namespace vf
