#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <detail/trace.hpp>
#include <detail/window/window.hpp>
#include <vulkify/instance/instance_create_info.hpp>
#include <vulkify/instance/monitor.hpp>

namespace vf {
namespace {
void attach_callbacks(GLFWwindow* w) {
	glfwSetWindowCloseCallback(w, [](GLFWwindow* w) {
		if (g_window.match(w)) {
			glfwSetWindowShouldClose(w, GLFW_TRUE);
			if (g_window.events->has_space()) { g_window.events->push_back({{}, EventType::eClose}); }
		}
	});
	glfwSetWindowIconifyCallback(w, [](GLFWwindow* w, int v) {
		if (g_window.match(w) && g_window.events->has_space()) { g_window.events->push_back({v == GLFW_TRUE, EventType::eIconify}); }
	});
	glfwSetWindowFocusCallback(w, [](GLFWwindow* w, int v) {
		if (g_window.match(w) && g_window.events->has_space()) { g_window.events->push_back({v == GLFW_TRUE, EventType::eFocus}); }
	});
	glfwSetWindowMaximizeCallback(w, [](GLFWwindow* w, int v) {
		if (g_window.match(w) && g_window.events->has_space()) { g_window.events->push_back({v == GLFW_TRUE, EventType::eMaximize}); }
	});
	glfwSetCursorEnterCallback(w, [](GLFWwindow* w, int v) {
		if (g_window.match(w) && g_window.events->has_space()) { g_window.events->push_back({v == GLFW_TRUE, EventType::eCursorEnter}); }
	});
	glfwSetWindowPosCallback(w, [](GLFWwindow* w, int x, int y) {
		if (g_window.match(w) && g_window.events->has_space()) { g_window.events->push_back({glm::ivec2(x, y), EventType::eMove}); }
	});
	glfwSetWindowSizeCallback(w, [](GLFWwindow* w, int x, int y) {
		if (g_window.match(w) && g_window.events->has_space()) { g_window.events->push_back({glm::ivec2(x, y), EventType::eWindowResize}); }
	});
	glfwSetFramebufferSizeCallback(w, [](GLFWwindow* w, int x, int y) {
		if (g_window.match(w) && g_window.events->has_space()) { g_window.events->push_back({glm::ivec2(x, y), EventType::eFramebufferResize}); }
	});
	glfwSetCursorPosCallback(w, [](GLFWwindow* w, double x, double y) {
		if (g_window.match(w) && g_window.events->has_space()) {
			g_input.position = glm::vec2{x, y};
			g_window.events->push_back({g_input.position, EventType::eCursorPos});
		}
	});
	glfwSetScrollCallback(w, [](GLFWwindow* w, double x, double y) {
		if (g_window.match(w) && g_window.events->has_space()) {
			g_input.scroll = glm::vec2(x, y);
			g_window.events->push_back({g_input.scroll, EventType::eScroll});
		}
	});
	glfwSetKeyCallback(w, [](GLFWwindow* w, int key, int, int action, int mods) {
		if (g_window.match(w) && g_window.events->has_space()) {
			auto const k = key < 0 || key > static_cast<int>(vf::Key::eCOUNT_) ? vf::Key::eUnknown : static_cast<vf::Key>(key);
			auto const key_event = KeyEvent{k, static_cast<Action>(action), static_cast<Mods::type>(mods)};
			g_window.events->push_back({key_event, EventType::eKey});
			g_input(key_event);
		}
	});
	glfwSetMouseButtonCallback(w, [](GLFWwindow* w, int button, int action, int mods) {
		if (g_window.match(w) && g_window.events->has_space()) {
			auto const key = static_cast<Key>(button + static_cast<int>(Key::eMouseButtonBegin));
			auto const key_event = KeyEvent{key, static_cast<Action>(action), static_cast<Mods::type>(mods)};
			g_window.events->push_back({key_event, EventType::eMouseButton});
			g_input(key_event);
		}
	});

	glfwSetCharCallback(w, [](GLFWwindow* w, std::uint32_t scancode) {
		if (g_window.match(w) && g_window.scancodes->has_space()) { g_window.scancodes->push_back(scancode); }
	});
	glfwSetDropCallback(w, [](GLFWwindow* w, int count, char const** paths) {
		if (g_window.match(w)) {
			for (int i = 0; i < count; ++i) {
				if (auto const path = paths[i]) { g_window.file_drops->push_back(path); }
			}
		}
	});
}

void detach_callbacks(GLFWwindow* w) {
	glfwSetWindowCloseCallback(w, {});
	glfwSetWindowIconifyCallback(w, {});
	glfwSetWindowFocusCallback(w, {});
	glfwSetWindowMaximizeCallback(w, {});
	glfwSetCursorEnterCallback(w, {});
	glfwSetWindowPosCallback(w, {});
	glfwSetWindowSizeCallback(w, {});
	glfwSetFramebufferSizeCallback(w, {});
	glfwSetCursorPosCallback(w, {});
	glfwSetScrollCallback(w, {});
	glfwSetKeyCallback(w, {});
	glfwSetMouseButtonCallback(w, {});

	glfwSetCharCallback(w, {});
	glfwSetDropCallback(w, {});
}

template <typename T, typename F, typename U = GLFWwindow*>
glm::tvec2<T> get_glfw_vec(U w, F f) {
	T x, y;
	f(w, &x, &y);
	return {x, y};
}

constexpr CursorMode to_cursor_mode(int mode) {
	switch (mode) {
	case GLFW_CURSOR_DISABLED: return CursorMode::eDisabled;
	case GLFW_CURSOR_HIDDEN: return CursorMode::eHidden;
	case GLFW_CURSOR_NORMAL:
	default: return CursorMode::eStandard;
	}
}

constexpr int from(CursorMode mode) {
	switch (mode) {
	case CursorMode::eDisabled: return GLFW_CURSOR_DISABLED;
	case CursorMode::eHidden: return GLFW_CURSOR_HIDDEN;
	case CursorMode::eStandard:
	default: return GLFW_CURSOR_NORMAL;
	}
}

VideoMode make_video_mode(GLFWvidmode const* mode) {
	auto const bitDepth = glm::ivec3(mode->redBits, mode->greenBits, mode->blueBits);
	auto const extent = glm::ivec2(mode->width, mode->height);
	return {extent, bitDepth, static_cast<std::uint32_t>(mode->refreshRate)};
}

std::vector<VideoMode> make_video_modes(GLFWvidmode const* modes, int count) {
	auto ret = std::vector<VideoMode>{};
	ret.reserve(static_cast<std::size_t>(count));
	for (int i = 0; i < count; ++i) { ret.push_back(make_video_mode(&modes[i])); }
	return ret;
}

Monitor make_monitor(GLFWmonitor* monitor) {
	int count;
	auto supported = glfwGetVideoModes(monitor, &count);
	auto position = get_glfw_vec<int>(monitor, &glfwGetMonitorPos);
	auto szName = glfwGetMonitorName(monitor);
	auto name = szName ? std::string(szName) : std::string();
	return {std::move(name), make_video_mode(glfwGetVideoMode(monitor)), make_video_modes(supported, count), position, monitor};
}
} // namespace

static_assert(sizeof(GamepadState) == sizeof(GLFWgamepadstate) && alignof(GamepadState) == alignof(GLFWgamepadstate));

void GamepadStorage::operator()() {
	for (int i = 0; i < Gamepad::max_id_v; ++i) {
		GLFWgamepadstate state;
		if (glfwGetGamepadState(GLFW_JOYSTICK_1 + i, &state)) {
			std::memcpy(&states[i], &state, sizeof(state));
		} else {
			states[i] = {};
		}
	}
}

void InputStorage::next() {
	auto previous = key_states;
	key_states = {};
	scroll = {};
	for (std::size_t i = 0; i < std::size(previous.key_states); ++i) {
		switch (previous.key_states[i]) {
		case KeyState::ePressed:
		case KeyState::eRepeated:
		case KeyState::eHeld: key_states.key_states[i] = KeyState::eHeld; break;
		default: break;
		}
	}
}

void InputStorage::operator()(KeyEvent const& key) {
	auto& st = key_states.key_states[static_cast<int>(key.key)];
	switch (key.action) {
	case Action::ePress: st = KeyState::ePressed; break;
	case Action::eRepeat: st = KeyState::eRepeated; break;
	case Action::eRelease: st = KeyState::eReleased; break;
	}
}

void Window::Deleter::operator()(Window const& window) const {
	detach_callbacks(static_cast<GLFWwindow*>(window.win));
	glfwDestroyWindow(static_cast<GLFWwindow*>(window.win));
}

void Window::Deleter::operator()(Instance const&) const { glfwTerminate(); }

UniqueWindow make_window(InstanceCreateInfo const& info, Window::Instance& instance) {
	auto const title = info.title ? info.title : "";
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_DECORATED, info.window_flags.test(WindowFlag::eBorderless) ? GLFW_FALSE : GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, info.instance_flags.test(InstanceFlag::eAutoShow) ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_MAXIMIZED, info.window_flags.test(WindowFlag::eMaximized) ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_RESIZABLE, info.window_flags.test(WindowFlag::eResizable) ? GLFW_TRUE : GLFW_FALSE);
	auto ret = glfwCreateWindow(static_cast<int>(info.extent.x), static_cast<int>(info.extent.y), title, nullptr, nullptr);
	attach_callbacks(ret);
	return Window{ret, &instance};
}

Result<std::shared_ptr<UniqueWindowInstance>> get_or_make_window_instance() {
	static std::weak_ptr<UniqueWindowInstance> s_ret{};
	if (auto lock = s_ret.lock()) { return lock; }
	if (!glfwInit()) { return Error::eGlfwFailure; }
	if (!glfwVulkanSupported()) {
		glfwTerminate();
		return Error::eNoVulkanSupport;
	}
#if defined(VULKIFY_DEBUG_TRACE)
	glfwSetErrorCallback([](int code, char const* szDesc) { VF_TRACEE("vf::Context", "GLFW Error [{}]: {}", code, szDesc); });
#endif
	auto ret = std::make_shared<UniqueWindowInstance>(true);
	s_ret = ret;
	return ret;
}

std::vector<char const*> Window::Instance::extensions() {
	static auto ret = std::vector<char const*>{};
	if (ret.empty()) {
		auto count = std::uint32_t{};
		auto exts = glfwGetRequiredInstanceExtensions(&count);
		for (std::uint32_t i = 0; i < count; ++i) { ret.push_back(exts[i]); }
	}
	return ret;
}

bool Window::make_surface(void* p_inst, void* p_surface) const {
	auto ret = false;
	auto* inst = static_cast<VkInstance*>(p_inst);
	auto* surface = static_cast<VkSurfaceKHR*>(p_surface);
	assert(inst && surface);
	auto const res = glfwCreateWindowSurface(*inst, static_cast<GLFWwindow*>(win), {}, surface);
	ret = res == VK_SUCCESS;
	return ret;
}

bool Window::closing() const {
	auto ret = false;
	ret = glfwWindowShouldClose(static_cast<GLFWwindow*>(win));
	return ret;
}

glm::ivec2 Window::framebuffer_size() const {
	auto ret = glm::ivec2{};
	ret = get_glfw_vec<int>(static_cast<GLFWwindow*>(win), &glfwGetFramebufferSize);
	return ret;
}

glm::ivec2 Window::window_size() const {
	auto ret = glm::ivec2{};
	ret = get_glfw_vec<int>(static_cast<GLFWwindow*>(win), &glfwGetWindowSize);
	return ret;
}

glm::ivec2 Window::position() const {
	auto ret = glm::ivec2{};
	ret = get_glfw_vec<int>(static_cast<GLFWwindow*>(win), &glfwGetWindowPos);
	return ret;
}

glm::vec2 Window::cursor_position() const {
	auto ret = glm::vec2{};
	ret = get_glfw_vec<double>(static_cast<GLFWwindow*>(win), &glfwGetCursorPos);
	return ret;
}

glm::vec2 Window::content_scale() const {
	auto ret = glm::vec2{};
	ret = get_glfw_vec<float>(static_cast<GLFWwindow*>(win), &glfwGetWindowContentScale);
	return ret;
}

CursorMode Window::cursor_mode() const {
	auto ret = CursorMode{};
	ret = to_cursor_mode(glfwGetInputMode(static_cast<GLFWwindow*>(win), GLFW_CURSOR));
	return ret;
}

MonitorList Window::monitors() const {
	auto ret = MonitorList{};
	int count;
	auto monitors = glfwGetMonitors(&count);
	if (count <= 0) { return {}; }
	ret.primary = make_monitor(monitors[0]);
	ret.others.reserve(static_cast<std::size_t>(count - 1));
	for (int i = 1; i < count; ++i) { ret.others.push_back(make_monitor(monitors[i])); }
	return ret;
}

WindowFlags Window::flags() const {
	auto ret = WindowFlags{};
	ret.assign(WindowFlag::eBorderless, !glfwGetWindowAttrib(static_cast<GLFWwindow*>(win), GLFW_DECORATED));
	ret.assign(WindowFlag::eResizable, glfwGetWindowAttrib(static_cast<GLFWwindow*>(win), GLFW_RESIZABLE));
	ret.assign(WindowFlag::eFloating, glfwGetWindowAttrib(static_cast<GLFWwindow*>(win), GLFW_FLOATING));
	ret.assign(WindowFlag::eAutoIconify, glfwGetWindowAttrib(static_cast<GLFWwindow*>(win), GLFW_AUTO_ICONIFY));
	ret.assign(WindowFlag::eMaximized, glfwGetWindowAttrib(static_cast<GLFWwindow*>(win), GLFW_MAXIMIZED));
	return ret;
}

void Window::show() { glfwShowWindow(static_cast<GLFWwindow*>(win)); }
void Window::hide() { glfwHideWindow(static_cast<GLFWwindow*>(win)); }
void Window::close() { glfwSetWindowShouldClose(static_cast<GLFWwindow*>(win), GLFW_TRUE); }
void Window::poll() { glfwPollEvents(); }
void Window::position(glm::ivec2 pos) { glfwSetWindowPos(static_cast<GLFWwindow*>(win), pos.x, pos.y); }
void Window::set_window_size(glm::ivec2 size) { glfwSetWindowSize(static_cast<GLFWwindow*>(win), size.x, size.y); }
void Window::set_cursor_mode(CursorMode mode) { glfwSetInputMode(static_cast<GLFWwindow*>(win), GLFW_CURSOR, from(mode)); }

void Window::lock_aspect_ratio(bool lock) {
	if (lock) {
		glm::ivec2 const extent = window_size();
		glfwSetWindowAspectRatio(static_cast<GLFWwindow*>(win), extent.x, extent.y);
	} else {
		glfwSetWindowAspectRatio(static_cast<GLFWwindow*>(win), GLFW_DONT_CARE, GLFW_DONT_CARE);
	}
}

void Window::update(WindowFlags set, WindowFlags unset) {
	auto update_attrib = [&](WindowFlag flag, int attrib, bool if_set) {
		if (set.test(flag)) { glfwSetWindowAttrib(static_cast<GLFWwindow*>(win), attrib, if_set ? GLFW_TRUE : GLFW_FALSE); }
		if (unset.test(flag)) { glfwSetWindowAttrib(static_cast<GLFWwindow*>(win), attrib, if_set ? GLFW_FALSE : GLFW_TRUE); }
	};
	update_attrib(WindowFlag::eBorderless, GLFW_DECORATED, false);
	update_attrib(WindowFlag::eFloating, GLFW_FLOATING, true);
	update_attrib(WindowFlag::eResizable, GLFW_RESIZABLE, true);
	update_attrib(WindowFlag::eAutoIconify, GLFW_AUTO_ICONIFY, true);
	auto const maximized = glfwGetWindowAttrib(static_cast<GLFWwindow*>(win), GLFW_MAXIMIZED);
	if (maximized) {
		if (unset.test(WindowFlag::eMaximized)) { glfwRestoreWindow(static_cast<GLFWwindow*>(win)); }
	} else {
		if (set.test(WindowFlag::eMaximized)) { glfwMaximizeWindow(static_cast<GLFWwindow*>(win)); }
	}
}

Cursor Window::make_cursor(Icon icon) {
	auto const extent = glm::ivec2(icon.bitmap.extent);
	auto const pixels = const_cast<unsigned char*>(reinterpret_cast<unsigned char const*>(icon.bitmap.data.data()));
	auto const img = GLFWimage{extent.x, extent.y, pixels};
	auto const glfw_cursor = glfwCreateCursor(&img, icon.hotspot.x, icon.hotspot.y);
	if (glfw_cursor) {
		auto const ret = Cursor{++instance->cursors.next_id};
		instance->cursors.cursors.insert_or_assign(ret, glfw_cursor);
		return ret;
	}
	return {};
}

void Window::destroy_cursor(Cursor cursor) { instance->cursors.cursors.erase(cursor); }

bool Window::set_cursor(Cursor cursor) {
	if (cursor == Cursor{}) {
		glfwSetCursor(static_cast<GLFWwindow*>(win), {});
		return true;
	}
	auto it = instance->cursors.cursors.find(cursor);
	if (it == instance->cursors.cursors.end()) { return false; }
	glfwSetCursor(static_cast<GLFWwindow*>(win), static_cast<GLFWcursor*>(it->second));
	return true;
}

void Window::set_icons(std::span<Icon const> icons) {
	if (icons.empty()) {
		glfwSetWindowIcon(static_cast<GLFWwindow*>(win), 0, {});
		return;
	}
	auto vec = std::vector<GLFWimage>{};
	vec.reserve(icons.size());
	for (auto const& icon : icons) {
		auto const extent = glm::ivec2(icon.bitmap.extent);
		auto const pixels = const_cast<unsigned char*>(reinterpret_cast<unsigned char const*>(icon.bitmap.data.data()));
		vec.push_back({extent.x, extent.y, pixels});
	}
	glfwSetWindowIcon(static_cast<GLFWwindow*>(win), static_cast<int>(vec.size()), vec.data());
}

void Window::set_windowed(Extent extent) {
	auto const ext = glm::ivec2(extent);
	glfwSetWindowMonitor(static_cast<GLFWwindow*>(win), nullptr, 0, 0, ext.x, ext.y, 0);
}

void Window::set_fullscreen(Monitor const& monitor, Extent resolution) {
	auto gmonitor = reinterpret_cast<GLFWmonitor*>(monitor.handle);
	auto const vmode = glfwGetVideoMode(gmonitor);
	if (!vmode) { return; }
	auto res = glm::ivec2(resolution);
	if (resolution.x == 0 || resolution.y == 0) { resolution = {vmode->width, vmode->height}; }
	glfwSetWindowMonitor(static_cast<GLFWwindow*>(win), gmonitor, 0, 0, res.x, res.y, vmode->refreshRate);
}

GamepadMap Window::gamepads() {
	auto ret = GamepadMap{};
	for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_16; ++i) { ret.map[i] = glfwJoystickIsGamepad(i) == GLFW_TRUE; }
	return ret;
}

void Window::update_gamepad_mappings(char const* text) { glfwUpdateGamepadMappings(text); }

bool Window::is_gamepad(int id) {
	auto ret = false;
	ret = glfwJoystickIsGamepad(GLFW_JOYSTICK_1 + id) == GLFW_TRUE;
	return ret;
}

char const* Window::gamepad_name(int id) {
	auto ret = glfwGetGamepadName(id);
	if (!ret) { ret = glfwGetJoystickName(id); }
	return ret ? ret : "";
}

bool Window::is_pressed(int id, Gamepad::Button button) {
	auto const& buttons = g_gamepads.states[id].buttons;
	auto const index = static_cast<int>(button);
	if (index >= static_cast<int>(std::size(buttons))) { return false; }
	return buttons[index] == GLFW_PRESS;
}

float Window::value(int id, Gamepad::Axis axis) {
	auto const& axes = g_gamepads.states[id].axes;
	auto const index = static_cast<int>(axis);
	if (index >= static_cast<int>(std::size(axes))) { return false; }
	float const coeff = axis == GamepadAxis::eLeftY || axis == GamepadAxis::eRightY ? -1.0f : 1.0f;
	return coeff * axes[index];
}
} // namespace vf
