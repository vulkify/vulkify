#pragma once
#include <vulkify/core/time.hpp>
#include <vulkify/graphics/primitive.hpp>
#include <vulkify/instance/instance.hpp>

namespace vf {
///
/// \brief Frame to render to (and present)
///
class Frame {
  public:
	using Poll = Instance::Poll;

	Frame() = default;

	Time dt() const { return m_dt; }
	Poll const& poll() const { return m_poll; }

	void draw(Primitive const& primitive) const { primitive.draw(m_surface); }

	Surface const& surface() const { return m_surface; }

  private:
	Frame(Surface&& surface, Poll poll, Time dt) noexcept : m_surface(std::move(surface)), m_poll(poll), m_dt(dt) {}

	Surface m_surface{};
	Poll m_poll{};
	Time m_dt{};

	friend class Context;
};
} // namespace vf
