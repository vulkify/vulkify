#pragma once
#include <ktl/fixed_vector.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/core/unique.hpp>
#include <vulkify/instance/cursor.hpp>
#include <vulkify/instance/event.hpp>
#include <vulkify/instance/gamepad.hpp>
#include <vulkify/instance/icon.hpp>
#include <vulkify/instance/instance_enums.hpp>
#include <string>
#include <unordered_map>

namespace vf {
struct InstanceCreateInfo;
struct Monitor;
struct MonitorList;

struct Dummy {};

struct GamepadState {
	unsigned char buttons[15];
	float axes[6];
};

using EventsStorage = ktl::fixed_vector<Event, 16>;
using ScancodeStorage = ktl::fixed_vector<std::uint32_t, 16>;
using FileDropStorage = std::vector<std::string>;

struct CursorStorage {
	struct Hasher {
		std::size_t operator()(Cursor cursor) const { return std::hash<std::uint64_t>{}(cursor.handle); }
	};

	std::unordered_map<Cursor, void*, Hasher> cursors{};
	std::uint64_t next{};
};

struct GamepadStorage {
	GamepadState states[Gamepad::max_id_v]{};

	void operator()();
};

struct Window {
	struct Instance;

	void* win{};
	Instance* instance{};

	EventsStorage events{};
	ScancodeStorage scancodes{};
	FileDropStorage fileDrops{};

	bool operator==(Window const& rhs) const { return win == rhs.win; }

	bool makeSurface(void* p_instance, void* p_surface) const;

	bool closing() const;
	glm::ivec2 framebufferSize() const;
	glm::ivec2 windowSize() const;
	glm::ivec2 position() const;
	glm::vec2 cursorPos() const;
	glm::vec2 contentScale() const;
	CursorMode cursorMode() const;
	MonitorList monitors() const;
	WindowFlags flags() const;

	void show();
	void hide();
	void close();
	void poll();

	void position(glm::ivec2 pos);
	void windowSize(glm::ivec2 size);
	void cursorMode(CursorMode mode);
	void update(WindowFlags set, WindowFlags unset);

	Cursor makeCursor(Icon icon);
	void destroyCursor(Cursor cursor);
	bool setCursor(Cursor cursor);

	void setIcons(std::span<Icon const> icons);
	void setWindowed(Extent extent);
	void setFullscreen(Monitor const& monitor, Extent resolution);

	static GamepadMap gamepads();
	static void updateGamepadMappings(char const* text);
	static bool isGamepad(int id);
	static char const* gamepadName(int id);
	static bool isPressed(int id, Gamepad::Button button);
	static float value(int id, Gamepad::Axis axis);

	struct Deleter {
		void operator()(Window const& window) const;
		void operator()(Instance const&) const;
	};
};

using UniqueWindow = Unique<Window, Window::Deleter>;
UniqueWindow makeWindow(InstanceCreateInfo const& info, Window::Instance& instance);

struct Window::Instance {
	CursorStorage cursors{};
	bool active{};

	Instance(bool active = false) : active(active) {}

	operator bool() const { return active; }
	bool operator==(Instance const& rhs) const { return active == rhs.active; }

	static std::vector<char const*> extensions();
};

using UniqueWindowInstance = Unique<Window::Instance, Window::Deleter>;
Result<std::shared_ptr<UniqueWindowInstance>> getOrMakeWindowInstance();

struct WindowView {
	void* window{};
	EventsStorage* events{};
	ScancodeStorage* scancodes{};
	FileDropStorage* fileDrops{};

	bool match(void const* w) const { return w == window && events && scancodes && fileDrops; }
};

inline WindowView g_window{};
inline GamepadStorage g_gamepads{};
} // namespace vf
