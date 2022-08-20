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
	FileDropStorage file_drops{};

	bool operator==(Window const& rhs) const { return win == rhs.win; }

	bool make_surface(void* p_instance, void* p_surface) const;

	bool closing() const;
	glm::ivec2 framebuffer_size() const;
	glm::ivec2 window_size() const;
	glm::ivec2 position() const;
	glm::vec2 cursor_position() const;
	glm::vec2 content_scale() const;
	CursorMode cursor_mode() const;
	MonitorList monitors() const;
	WindowFlags flags() const;

	void show();
	void hide();
	void close();
	void poll();

	void position(glm::ivec2 pos);
	void set_window_size(glm::ivec2 size);
	void set_cursor_mode(CursorMode mode);
	void update(WindowFlags set, WindowFlags unset);

	Cursor make_cursor(Icon icon);
	void destroy_cursor(Cursor cursor);
	bool set_cursor(Cursor cursor);

	void set_icons(std::span<Icon const> icons);
	void set_windowed(Extent extent);
	void set_fullscreen(Monitor const& monitor, Extent resolution);
	void lock_aspect_ratio(bool lock);

	static GamepadMap gamepads();
	static void update_gamepad_mappings(char const* text);
	static bool is_gamepad(int id);
	static char const* gamepad_name(int id);
	static bool is_pressed(int id, Gamepad::Button button);
	static float value(int id, Gamepad::Axis axis);

	struct Deleter {
		void operator()(Window const& window) const;
		void operator()(Instance const&) const;
	};
};

using UniqueWindow = Unique<Window, Window::Deleter>;
UniqueWindow make_window(InstanceCreateInfo const& info, Window::Instance& instance);

struct Window::Instance {
	CursorStorage cursors{};
	bool active{};

	Instance(bool active = false) : active(active) {}

	operator bool() const { return active; }
	bool operator==(Instance const& rhs) const { return active == rhs.active; }

	static std::vector<char const*> extensions();
};

using UniqueWindowInstance = Unique<Window::Instance, Window::Deleter>;
Result<std::shared_ptr<UniqueWindowInstance>> get_or_make_window_instance();

struct WindowView {
	void* window{};
	EventsStorage* events{};
	ScancodeStorage* scancodes{};
	FileDropStorage* file_drops{};

	bool match(void const* w) const { return w == window && events && scancodes && file_drops; }
};

inline WindowView g_window{};
inline GamepadStorage g_gamepads{};
} // namespace vf
