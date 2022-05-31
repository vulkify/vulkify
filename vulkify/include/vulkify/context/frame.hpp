#pragma once
#include <vulkify/core/time.hpp>
#include <vulkify/graphics/handles.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <vulkify/graphics/surface.hpp>
#include <vulkify/instance/event_queue.hpp>

namespace vf {
///
/// \brief Frame to render to (and present)
///
class Frame {
  public:
	Frame() = default;

	Time dt() const { return m_dt; }
	EventQueue const& poll() const { return m_poll; }

	void draw(Primitive const& primitive, Pipeline const& pipeline = {}) const { primitive.draw(m_surface, pipeline); }
	void draw(Primitive const& primitive, ShaderHandle shader) const { draw(primitive, {.shader = shader}); }

	Surface const& surface() const { return m_surface; }

  private:
	Frame(Surface&& surface, EventQueue poll, Time dt) noexcept : m_surface(std::move(surface)), m_poll(poll), m_dt(dt) {}

	Surface m_surface{};
	EventQueue m_poll{};
	Time m_dt{};

	friend class Context;
};
} // namespace vf
