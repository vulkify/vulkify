#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <detail/trace.hpp>
#include <detail/window/window.hpp>
#include <vulkify/instance/instance_create_info.hpp>
#include <vulkify/instance/monitor.hpp>

namespace vf {
namespace {
void attachCallbacks(GLFWwindow* w) {
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
		if (g_window.match(w) && g_window.events->has_space()) { g_window.events->push_back({glm::vec2(x, y), EventType::eCursorPos}); }
	});
	glfwSetScrollCallback(w, [](GLFWwindow* w, double x, double y) {
		if (g_window.match(w) && g_window.events->has_space()) { g_window.events->push_back({glm::vec2(x, y), EventType::eScroll}); }
	});
	glfwSetKeyCallback(w, [](GLFWwindow* w, int key, int, int action, int mods) {
		if (g_window.match(w) && g_window.events->has_space()) {
			auto const keyEvent = KeyEvent{static_cast<Key>(key), static_cast<Action>(action), static_cast<Mods::type>(mods)};
			g_window.events->push_back({keyEvent, EventType::eKey});
		}
	});
	glfwSetMouseButtonCallback(w, [](GLFWwindow* w, int button, int action, int mods) {
		if (g_window.match(w) && g_window.events->has_space()) {
			auto const key = static_cast<Key>(button + static_cast<int>(Key::eMouseButtonBegin));
			auto const keyEvent = KeyEvent{key, static_cast<Action>(action), static_cast<Mods::type>(mods)};
			g_window.events->push_back({keyEvent, EventType::eMouseButton});
		}
	});

	glfwSetCharCallback(w, [](GLFWwindow* w, std::uint32_t scancode) {
		if (g_window.match(w) && g_window.scancodes->has_space()) { g_window.scancodes->push_back(scancode); }
	});
	glfwSetDropCallback(w, [](GLFWwindow* w, int count, char const** paths) {
		if (g_window.match(w)) {
			for (int i = 0; i < count; ++i) {
				if (auto const path = paths[i]) { g_window.fileDrops->push_back(path); }
			}
		}
	});
}

void detachCallbacks(GLFWwindow* w) {
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
glm::tvec2<T> getGlfwVec(U w, F f) {
	T x, y;
	f(w, &x, &y);
	return {x, y};
}

constexpr CursorMode castCursorMode(int mode) {
	switch (mode) {
	case GLFW_CURSOR_DISABLED: return CursorMode::eDisabled;
	case GLFW_CURSOR_HIDDEN: return CursorMode::eHidden;
	case GLFW_CURSOR_NORMAL:
	default: return CursorMode::eStandard;
	}
}

constexpr int cast(CursorMode mode) {
	switch (mode) {
	case CursorMode::eDisabled: return GLFW_CURSOR_DISABLED;
	case CursorMode::eHidden: return GLFW_CURSOR_HIDDEN;
	case CursorMode::eStandard:
	default: return GLFW_CURSOR_NORMAL;
	}
}

VideoMode makeVideoMode(GLFWvidmode const* mode) {
	auto const bitDepth = glm::ivec3(mode->redBits, mode->greenBits, mode->blueBits);
	auto const extent = glm::ivec2(mode->width, mode->height);
	return {extent, bitDepth, static_cast<std::uint32_t>(mode->refreshRate)};
}

std::vector<VideoMode> makeVideoModes(GLFWvidmode const* modes, int count) {
	auto ret = std::vector<VideoMode>{};
	ret.reserve(static_cast<std::size_t>(count));
	for (int i = 0; i < count; ++i) { ret.push_back(makeVideoMode(&modes[i])); }
	return ret;
}

Monitor makeMonitor(GLFWmonitor* monitor) {
	int count;
	auto supported = glfwGetVideoModes(monitor, &count);
	auto position = getGlfwVec<int>(monitor, &glfwGetMonitorPos);
	auto szName = glfwGetMonitorName(monitor);
	auto name = szName ? std::string(szName) : std::string();
	return {std::move(name), makeVideoMode(glfwGetVideoMode(monitor)), makeVideoModes(supported, count), position, monitor};
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

void Window::Deleter::operator()(Window const& window) const {
	detachCallbacks(window.win.get<GLFWwindow*>());
	glfwDestroyWindow(window.win.get<GLFWwindow*>());
}

void Window::Deleter::operator()(Instance const&) const { glfwTerminate(); }

UniqueWindow makeWindow(InstanceCreateInfo const& info, Window::Instance& instance) {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_DECORATED, info.windowFlags.test(WindowFlag::eBorderless) ? GLFW_FALSE : GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, info.instanceFlags.test(InstanceFlag::eAutoShow) ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_MAXIMIZED, info.windowFlags.test(WindowFlag::eMaximized) ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_RESIZABLE, info.windowFlags.test(WindowFlag::eResizable) ? GLFW_TRUE : GLFW_FALSE);
	auto ret = glfwCreateWindow(static_cast<int>(info.extent.x), static_cast<int>(info.extent.y), info.title, nullptr, nullptr);
	attachCallbacks(ret);
	return Window{ret, &instance};
}

Result<std::shared_ptr<UniqueWindowInstance>> getOrMakeWindowInstance() {
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

bool Window::makeSurface(ErasedPtr vkinst, ErasedPtr out_vksurface) const {
	auto ret = false;
	auto* inst = vkinst.get<VkInstance*>();
	auto* surface = out_vksurface.get<VkSurfaceKHR*>();
	assert(inst && surface);
	auto const res = glfwCreateWindowSurface(*inst, win.get<GLFWwindow*>(), {}, surface);
	ret = res == VK_SUCCESS;
	return ret;
}

bool Window::closing() const {
	auto ret = false;
	ret = glfwWindowShouldClose(win.get<GLFWwindow*>());
	return ret;
}

glm::ivec2 Window::framebufferSize() const {
	auto ret = glm::ivec2{};
	ret = getGlfwVec<int>(win.get<GLFWwindow*>(), &glfwGetFramebufferSize);
	return ret;
}

glm::ivec2 Window::windowSize() const {
	auto ret = glm::ivec2{};
	ret = getGlfwVec<int>(win.get<GLFWwindow*>(), &glfwGetWindowSize);
	return ret;
}

glm::ivec2 Window::position() const {
	auto ret = glm::ivec2{};
	ret = getGlfwVec<int>(win.get<GLFWwindow*>(), &glfwGetWindowPos);
	return ret;
}

glm::vec2 Window::cursorPos() const {
	auto ret = glm::vec2{};
	ret = getGlfwVec<double>(win.get<GLFWwindow*>(), &glfwGetCursorPos);
	return ret;
}

glm::vec2 Window::contentScale() const {
	auto ret = glm::vec2{};
	ret = getGlfwVec<float>(win.get<GLFWwindow*>(), &glfwGetWindowContentScale);
	return ret;
}

CursorMode Window::cursorMode() const {
	auto ret = CursorMode{};
	ret = castCursorMode(glfwGetInputMode(win.get<GLFWwindow*>(), GLFW_CURSOR));
	return ret;
}

MonitorList Window::monitors() const {
	auto ret = MonitorList{};
	int count;
	auto monitors = glfwGetMonitors(&count);
	if (count <= 0) { return {}; }
	ret.primary = makeMonitor(monitors[0]);
	ret.others.reserve(static_cast<std::size_t>(count - 1));
	for (int i = 1; i < count; ++i) { ret.others.push_back(makeMonitor(monitors[i])); }
	return ret;
}

WindowFlags Window::flags() const {
	auto ret = WindowFlags{};
	ret.assign(WindowFlag::eBorderless, !glfwGetWindowAttrib(win.get<GLFWwindow*>(), GLFW_DECORATED));
	ret.assign(WindowFlag::eResizable, glfwGetWindowAttrib(win.get<GLFWwindow*>(), GLFW_RESIZABLE));
	ret.assign(WindowFlag::eFloating, glfwGetWindowAttrib(win.get<GLFWwindow*>(), GLFW_FLOATING));
	ret.assign(WindowFlag::eAutoIconify, glfwGetWindowAttrib(win.get<GLFWwindow*>(), GLFW_AUTO_ICONIFY));
	ret.assign(WindowFlag::eMaximized, glfwGetWindowAttrib(win.get<GLFWwindow*>(), GLFW_MAXIMIZED));
	return ret;
}

void Window::show() { glfwShowWindow(win.get<GLFWwindow*>()); }
void Window::hide() { glfwHideWindow(win.get<GLFWwindow*>()); }
void Window::close() { glfwSetWindowShouldClose(win.get<GLFWwindow*>(), GLFW_TRUE); }
void Window::poll() { glfwPollEvents(); }
void Window::position(glm::ivec2 pos) { glfwSetWindowPos(win.get<GLFWwindow*>(), pos.x, pos.y); }
void Window::windowSize(glm::ivec2 size) { glfwSetWindowSize(win.get<GLFWwindow*>(), size.x, size.y); }
void Window::cursorMode(CursorMode mode) { glfwSetInputMode(win.get<GLFWwindow*>(), GLFW_CURSOR, cast(mode)); }

void Window::update(WindowFlags set, WindowFlags unset) {
	auto updateAttrib = [&](WindowFlag flag, int attrib, bool ifSet) {
		if (set.test(flag)) { glfwSetWindowAttrib(win.get<GLFWwindow*>(), attrib, ifSet ? GLFW_TRUE : GLFW_FALSE); }
		if (unset.test(flag)) { glfwSetWindowAttrib(win.get<GLFWwindow*>(), attrib, ifSet ? GLFW_FALSE : GLFW_TRUE); }
	};
	updateAttrib(WindowFlag::eBorderless, GLFW_DECORATED, false);
	updateAttrib(WindowFlag::eFloating, GLFW_FLOATING, true);
	updateAttrib(WindowFlag::eResizable, GLFW_RESIZABLE, true);
	updateAttrib(WindowFlag::eAutoIconify, GLFW_AUTO_ICONIFY, true);
	auto const maximized = glfwGetWindowAttrib(win.get<GLFWwindow*>(), GLFW_MAXIMIZED);
	if (maximized) {
		if (unset.test(WindowFlag::eMaximized)) { glfwRestoreWindow(win.get<GLFWwindow*>()); }
	} else {
		if (set.test(WindowFlag::eMaximized)) { glfwMaximizeWindow(win.get<GLFWwindow*>()); }
	}
}

Cursor Window::makeCursor(Icon icon) {
	auto const extent = glm::ivec2(icon.bitmap.extent);
	auto const pixels = const_cast<unsigned char*>(reinterpret_cast<unsigned char const*>(icon.bitmap.data.data()));
	auto const img = GLFWimage{extent.x, extent.y, pixels};
	auto const glfwCursor = glfwCreateCursor(&img, icon.hotspot.x, icon.hotspot.y);
	if (glfwCursor) {
		auto const ret = Cursor{++instance->cursors.next};
		instance->cursors.cursors.insert_or_assign(ret, glfwCursor);
		return ret;
	}
	return {};
}

void Window::destroyCursor(Cursor cursor) { instance->cursors.cursors.erase(cursor); }

bool Window::setCursor(Cursor cursor) {
	if (cursor == Cursor{}) {
		glfwSetCursor(win.get<GLFWwindow*>(), {});
		return true;
	}
	auto it = instance->cursors.cursors.find(cursor);
	if (it == instance->cursors.cursors.end()) { return false; }
	glfwSetCursor(win.get<GLFWwindow*>(), it->second.get<GLFWcursor*>());
	return true;
}

void Window::setIcons(std::span<Icon const> icons) {
	if (icons.empty()) {
		glfwSetWindowIcon(win.get<GLFWwindow*>(), 0, {});
		return;
	}
	auto vec = std::vector<GLFWimage>{};
	vec.reserve(icons.size());
	for (auto const& icon : icons) {
		auto const extent = glm::ivec2(icon.bitmap.extent);
		auto const pixels = const_cast<unsigned char*>(reinterpret_cast<unsigned char const*>(icon.bitmap.data.data()));
		vec.push_back({extent.x, extent.y, pixels});
	}
	glfwSetWindowIcon(win.get<GLFWwindow*>(), static_cast<int>(vec.size()), vec.data());
}

void Window::setWindowed(Extent extent) {
	auto const ext = glm::ivec2(extent);
	glfwSetWindowMonitor(win.get<GLFWwindow*>(), nullptr, 0, 0, ext.x, ext.y, 0);
}

void Window::setFullscreen(Monitor const& monitor, Extent resolution) {
	auto gmonitor = reinterpret_cast<GLFWmonitor*>(monitor.handle);
	auto const vmode = glfwGetVideoMode(gmonitor);
	if (!vmode) { return; }
	auto res = glm::ivec2(resolution);
	if (resolution.x == 0 || resolution.y == 0) { resolution = {vmode->width, vmode->height}; }
	glfwSetWindowMonitor(win.get<GLFWwindow*>(), gmonitor, 0, 0, res.x, res.y, vmode->refreshRate);
}

GamepadMap Window::gamepads() {
	auto ret = GamepadMap{};
	for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_16; ++i) { ret.map[i] = glfwJoystickIsGamepad(i) == GLFW_TRUE; }
	return ret;
}

void Window::updateGamepadMappings(char const* text) { glfwUpdateGamepadMappings(text); }

bool Window::isGamepad(int id) {
	auto ret = false;
	ret = glfwJoystickIsGamepad(GLFW_JOYSTICK_1 + id) == GLFW_TRUE;
	return ret;
}

char const* Window::gamepadName(int id) {
	auto ret = glfwGetGamepadName(id);
	if (!ret) { ret = glfwGetJoystickName(id); }
	return ret ? ret : "";
}

bool Window::isPressed(int id, Gamepad::Button button) {
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
