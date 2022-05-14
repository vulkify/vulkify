#pragma once
#include <glm/vec2.hpp>
#include <ktl/kvariant.hpp>
#include <vulkify/instance/event_type.hpp>
#include <vulkify/instance/key_event.hpp>

namespace vf {
///
/// \brief Value of an Event
///
using EventValue = ktl::kvariant<glm::ivec2, glm::vec2, KeyEvent, bool>;

///
/// \brief Event type and value
///
struct Event {
	EventValue value{};
	EventType type{};

	template <typename T>
	T const& get() const;

	template <EventType T>
	decltype(auto) get() const;
};

// impl

namespace detail {
template <EventType>
struct EventTypeMap;

template <>
struct EventTypeMap<EventType::eClose> {
	using type = void;
};

template <>
struct EventTypeMap<EventType::eIconify> {
	using type = bool;
};
template <>
struct EventTypeMap<EventType::eFocus> {
	using type = bool;
};
template <>
struct EventTypeMap<EventType::eMaximize> {
	using type = bool;
};
template <>
struct EventTypeMap<EventType::eCursorEnter> {
	using type = bool;
};

template <>
struct EventTypeMap<EventType::eMove> {
	using type = glm::ivec2;
};
template <>
struct EventTypeMap<EventType::eWindowResize> {
	using type = glm::ivec2;
};
template <>
struct EventTypeMap<EventType::eFramebufferResize> {
	using type = glm::ivec2;
};

template <>
struct EventTypeMap<EventType::eCursorPos> {
	using type = glm::vec2;
};
template <>
struct EventTypeMap<EventType::eScroll> {
	using type = glm::vec2;
};

template <>
struct EventTypeMap<EventType::eKey> {
	using type = KeyEvent;
};
template <>
struct EventTypeMap<EventType::eMouseButton> {
	using type = KeyEvent;
};
} // namespace detail

template <typename T>
T const& Event::get() const {
	return value.template get<T>();
}

template <EventType T>
decltype(auto) Event::get() const {
	return get<typename detail::EventTypeMap<T>::type>();
}
} // namespace vf
