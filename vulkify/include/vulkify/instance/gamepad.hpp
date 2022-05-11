#pragma once

namespace vf {
class Context;

enum class GamepadButton {
	eA,
	eB,
	eX,
	eY,
	eCross = eA,
	eCircle = eB,
	eSquare = eX,
	eTriangle = eY,

	eLeftBumper,
	eRightBumper,
	eBack,
	eStart,
	eGuide,
	eLeftThumb,
	eRightThumb,
	eDpadUp,
	eDpadRight,
	eDpadDown,
	eDpadLeft,
};

enum class GamepadAxis {
	eUnknown = -1,
	eLeftX = 0,
	eLeftY,
	eRightX,
	eRightY,
	eLeftTrigger,
	eRightTrigger,
};

struct GamepadMap;

struct Gamepad {
	using Id = int;
	static constexpr Id max_id_v{16};
	using Button = GamepadButton;
	using Axis = GamepadAxis;
	using Map = GamepadMap;

	static Map map();

	Id id{};

	explicit operator bool() const;
	char const* name() const;

	bool operator()(Button button) const;
	float operator()(Axis axis) const;
};

struct GamepadMap {
	bool map[Gamepad::max_id_v]{};

	bool connected(Gamepad::Id id) const { return map[id]; }
	std::size_t count() const;
};
} // namespace vf