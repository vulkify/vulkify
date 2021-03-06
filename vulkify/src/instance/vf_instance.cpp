#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <detail/vk_instance.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ktl/debug_trap.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/core/unique.hpp>

#include <vulkify/instance/headless_instance.hpp>
#include <vulkify/instance/vf_instance.hpp>
#include <memory>
#include <vector>

#include <detail/descriptor_set_factory.hpp>
#include <detail/pipeline_factory.hpp>
#include <detail/render_pass.hpp>
#include <detail/renderer.hpp>
#include <detail/rotator.hpp>
#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <detail/vk_surface.hpp>
#include <detail/vram.hpp>
#include <functional>

#include <glm/mat4x4.hpp>
#include <vulkify/graphics/bitmap.hpp>
#include <vulkify/graphics/camera.hpp>
#include <vulkify/graphics/geometry.hpp>
#include <iostream>

#include <vulkify/instance/gamepad.hpp>

#include <ttf/ft.hpp>

#include <ktl/ktl_version.hpp>

static_assert(ktl::version_v >= ktl::kversion{1, 4, 0});

namespace vf {
namespace {
using EventsStorage = ktl::fixed_vector<Event, Instance::max_events_v>;
using ScancodeStorage = ktl::fixed_vector<std::uint32_t, Instance::max_scancodes_v>;
using FileDropStorage = std::vector<std::string>;

inline constexpr auto empty_cstr_v = "";

struct CursorStorage {
	struct Hasher {
		std::size_t operator()(Cursor cursor) const { return std::hash<std::uint64_t>{}(cursor.handle); }
	};

	std::unordered_map<Cursor, GLFWcursor*, Hasher> cursors{};
	std::uint64_t next{};
};

struct {
	GLFWwindow* window{};
	EventsStorage* events{};
	ScancodeStorage* scancodes{};
	FileDropStorage* fileDrops{};

	bool match(GLFWwindow const* w) const { return w == window && events && scancodes && fileDrops; }
} g_glfw;

struct {
	GLFWgamepadstate states[Gamepad::max_id_v]{};

	void operator()() {
		for (int i = 0; i < Gamepad::max_id_v; ++i) {
			GLFWgamepadstate state;
			if (glfwGetGamepadState(GLFW_JOYSTICK_1 + i, &state)) {
				states[i] = state;
			} else {
				states[i] = {};
			}
		}
	}
} g_gamepads{};

template <typename Out, typename In = int>
constexpr glm::tvec2<Out> cast(glm::tvec2<In> const& in) {
	return {static_cast<Out>(in.x), static_cast<Out>(in.y)};
}

void attachCallbacks(GLFWwindow* w) {
	glfwSetWindowCloseCallback(w, [](GLFWwindow* w) {
		if (g_glfw.match(w)) {
			glfwSetWindowShouldClose(w, GLFW_TRUE);
			if (g_glfw.events->has_space()) { g_glfw.events->push_back({{}, EventType::eClose}); }
		}
	});
	glfwSetWindowIconifyCallback(w, [](GLFWwindow* w, int v) {
		if (g_glfw.match(w) && g_glfw.events->has_space()) { g_glfw.events->push_back({v == GLFW_TRUE, EventType::eIconify}); }
	});
	glfwSetWindowFocusCallback(w, [](GLFWwindow* w, int v) {
		if (g_glfw.match(w) && g_glfw.events->has_space()) { g_glfw.events->push_back({v == GLFW_TRUE, EventType::eFocus}); }
	});
	glfwSetWindowMaximizeCallback(w, [](GLFWwindow* w, int v) {
		if (g_glfw.match(w) && g_glfw.events->has_space()) { g_glfw.events->push_back({v == GLFW_TRUE, EventType::eMaximize}); }
	});
	glfwSetCursorEnterCallback(w, [](GLFWwindow* w, int v) {
		if (g_glfw.match(w) && g_glfw.events->has_space()) { g_glfw.events->push_back({v == GLFW_TRUE, EventType::eCursorEnter}); }
	});
	glfwSetWindowPosCallback(w, [](GLFWwindow* w, int x, int y) {
		if (g_glfw.match(w) && g_glfw.events->has_space()) { g_glfw.events->push_back({glm::ivec2(x, y), EventType::eMove}); }
	});
	glfwSetWindowSizeCallback(w, [](GLFWwindow* w, int x, int y) {
		if (g_glfw.match(w) && g_glfw.events->has_space()) { g_glfw.events->push_back({glm::ivec2(x, y), EventType::eWindowResize}); }
	});
	glfwSetFramebufferSizeCallback(w, [](GLFWwindow* w, int x, int y) {
		if (g_glfw.match(w) && g_glfw.events->has_space()) { g_glfw.events->push_back({glm::ivec2(x, y), EventType::eFramebufferResize}); }
	});
	glfwSetCursorPosCallback(w, [](GLFWwindow* w, double x, double y) {
		if (g_glfw.match(w) && g_glfw.events->has_space()) { g_glfw.events->push_back({glm::vec2(x, y), EventType::eCursorPos}); }
	});
	glfwSetScrollCallback(w, [](GLFWwindow* w, double x, double y) {
		if (g_glfw.match(w) && g_glfw.events->has_space()) { g_glfw.events->push_back({glm::vec2(x, y), EventType::eScroll}); }
	});
	glfwSetKeyCallback(w, [](GLFWwindow* w, int key, int, int action, int mods) {
		if (g_glfw.match(w) && g_glfw.events->has_space()) {
			auto const keyEvent = KeyEvent{static_cast<Key>(key), static_cast<Action>(action), static_cast<Mods::type>(mods)};
			g_glfw.events->push_back({keyEvent, EventType::eKey});
		}
	});
	glfwSetMouseButtonCallback(w, [](GLFWwindow* w, int button, int action, int mods) {
		if (g_glfw.match(w) && g_glfw.events->has_space()) {
			auto const key = static_cast<Key>(button + static_cast<int>(Key::eMouseButtonBegin));
			auto const keyEvent = KeyEvent{key, static_cast<Action>(action), static_cast<Mods::type>(mods)};
			g_glfw.events->push_back({keyEvent, EventType::eMouseButton});
		}
	});

	glfwSetCharCallback(w, [](GLFWwindow* w, std::uint32_t scancode) {
		if (g_glfw.match(w) && g_glfw.scancodes->has_space()) { g_glfw.scancodes->push_back(scancode); }
	});
	glfwSetDropCallback(w, [](GLFWwindow* w, int count, char const** paths) {
		if (g_glfw.match(w)) {
			for (int i = 0; i < count; ++i) {
				if (auto const path = paths[i]) { g_glfw.fileDrops->push_back(path); }
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
} // namespace

namespace {
struct Glfw {
	CursorStorage cursors{};
	bool active{};

	Glfw(bool active = false) : active(active) {}

	operator bool() const { return active; }
	bool operator==(Glfw const& rhs) const { return active == rhs.active; }

	static std::vector<char const*> instanceExtensions() {
		static auto ret = std::vector<char const*>{};
		if (ret.empty()) {
			auto count = std::uint32_t{};
			auto exts = glfwGetRequiredInstanceExtensions(&count);
			for (std::uint32_t i = 0; i < count; ++i) { ret.push_back(exts[i]); }
		}
		return ret;
	}

	struct Deleter {
		void operator()(Glfw const&) const { glfwTerminate(); }
	};
};

using UniqueGlfw = Unique<Glfw, Glfw::Deleter>;

struct Window {
	GLFWwindow* win{};
	EventsStorage events{};
	ScancodeStorage scancodes{};
	FileDropStorage fileDrops{};

	bool operator==(Window const& rhs) const { return win == rhs.win; }

	struct Deleter {
		void operator()(Window const& window) const {
			detachCallbacks(window.win);
			glfwDestroyWindow(window.win);
		}
	};
};

using UniqueWindow = Unique<Window, Window::Deleter>;

Result<std::shared_ptr<UniqueGlfw>> getOrMakeGlfw() {
	static std::weak_ptr<UniqueGlfw> s_glfw{};
	if (auto lock = s_glfw.lock()) { return lock; }
	if (!glfwInit()) { return Error::eGlfwFailure; }
	if (!glfwVulkanSupported()) {
		glfwTerminate();
		return Error::eNoVulkanSupport;
	}
	glfwSetErrorCallback([]([[maybe_unused]] int code, [[maybe_unused]] char const* szDesc) { VF_TRACEE("vf::Context", "GLFW Error [{}]: {}", code, szDesc); });
	auto ret = std::make_shared<UniqueGlfw>(true);
	s_glfw = ret;
	return ret;
}

UniqueWindow makeWindow(InstanceCreateInfo const& info) {
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_DECORATED, info.windowFlags.test(WindowFlag::eBorderless) ? GLFW_FALSE : GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, info.instanceFlags.test(InstanceFlag::eAutoShow) ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_MAXIMIZED, info.windowFlags.test(WindowFlag::eMaximized) ? GLFW_TRUE : GLFW_FALSE);
	glfwWindowHint(GLFW_RESIZABLE, info.windowFlags.test(WindowFlag::eResizable) ? GLFW_TRUE : GLFW_FALSE);
	auto ret = glfwCreateWindow(static_cast<int>(info.extent.x), static_cast<int>(info.extent.y), info.title, nullptr, nullptr);
	attachCallbacks(ret);
	return Window{ret};
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

glm::ivec2 getFramebufferSize(GLFWwindow* w) { return getGlfwVec<int>(w, &glfwGetFramebufferSize); }
glm::ivec2 getWindowSize(GLFWwindow* w) { return getGlfwVec<int>(w, &glfwGetWindowSize); }

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

namespace {
constexpr vk::Format textureFormat(vk::Format surface) { return Renderer::isSrgb(surface) ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm; }

VKDevice makeDevice(VKInstance const& instance) {
	auto const flags = instance.messenger ? VKDevice::Flag::eDebugMsgr : VKDevice::Flags{};
	return {instance.queue, instance.gpu.device, *instance.device, {&instance.util->defer}, &instance.util->mutex.queue, flags};
}

vk::UniqueDescriptorSetLayout makeSetLayout(vk::Device device, std::span<vk::DescriptorSetLayoutBinding const> bindings) {
	auto info = vk::DescriptorSetLayoutCreateInfo({}, static_cast<std::uint32_t>(bindings.size()), bindings.data());
	return device.createDescriptorSetLayoutUnique(info);
}

constexpr int getSamples(AntiAliasing aa) {
	switch (aa) {
	case AntiAliasing::e16x: return 16;
	case AntiAliasing::e8x: return 8;
	case AntiAliasing::e4x: return 4;
	case AntiAliasing::e2x: return 2;
	case AntiAliasing::eNone:
	default: return 1;
	}
}

std::vector<vk::UniqueDescriptorSetLayout> makeSetLayouts(vk::Device device) {
	using DSet = SetWriter;
	auto ret = std::vector<vk::UniqueDescriptorSetLayout>{};
	auto addSet = [&](vk::ShaderStageFlags bufferStages) {
		auto b0 = vk::DescriptorSetLayoutBinding(0, DSet::buffer_layouts_v[0].type, 1, bufferStages);
		auto b1 = vk::DescriptorSetLayoutBinding(1, DSet::buffer_layouts_v[1].type, 1, bufferStages);
		auto b2 = vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		vk::DescriptorSetLayoutBinding const binds[] = {b0, b1, b2};
		ret.push_back(makeSetLayout(device, binds));
	};

	// set 0: scene data
	{
		// binding 0: matrices
		auto b0 = vk::DescriptorSetLayoutBinding(0, DSet::buffer_layouts_v[0].type, 1, vk::ShaderStageFlagBits::eVertex);
		ret.push_back(makeSetLayout(device, {&b0, 1}));
	}
	// set 1: object data
	addSet(vk::ShaderStageFlagBits::eVertex);
	// set 2: custom
	addSet(vk::ShaderStageFlagBits::eFragment);
	return ret;
}

std::vector<vk::DescriptorSetLayout> makeSetLayouts(std::span<vk::UniqueDescriptorSetLayout const> layouts) {
	auto ret = std::vector<vk::DescriptorSetLayout>{};
	ret.reserve(layouts.size());
	for (auto const& layout : layouts) { ret.push_back(*layout); }
	return ret;
}

struct VIStorage {
	std::vector<vk::VertexInputBindingDescription> bindings{};
	std::vector<vk::VertexInputAttributeDescription> attributes{};

	VertexInput operator()() const { return {bindings, attributes}; }

	static VIStorage make() {
		auto ret = VIStorage{};
		auto vertex = vk::VertexInputBindingDescription(0, sizeof(Vertex));
		ret.bindings = {vertex};
		auto xy = vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, xy));
		auto uv = vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv));
		auto rgba = vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, rgba));
		ret.attributes = {xy, uv, rgba};
		return ret;
	}
};

struct FrameSync {
	vk::UniqueSemaphore draw{};
	vk::UniqueSemaphore present{};
	vk::UniqueFence drawn{};
	vk::UniqueFramebuffer framebuffer{};
	struct {
		vk::CommandBuffer primary{};
		vk::CommandBuffer secondary{};
	} cmd{};

	static FrameSync make(vk::Device device) {
		return {
			device.createSemaphoreUnique({}),
			device.createSemaphoreUnique({}),
			device.createFenceUnique({vk::FenceCreateFlagBits::eSignaled}),
		};
	}

	explicit operator bool() const { return draw.get(); }
	bool operator==(FrameSync const& rhs) const { return !draw && !rhs.draw; }

	VKSync sync() const { return {*draw, *present, *drawn}; }
};

struct SwapchainRenderer {
	Vram vram{};

	Renderer renderer{};
	Rotator<FrameSync> frameSync{};
	ImageCache msaaImage{};
	vk::UniqueCommandPool commandPool{};

	Framebuffer framebuffer{};
	VKImage present{};
	Rgba clear{};
	bool msaa{};

	static SwapchainRenderer make(Vram const& vram, vk::Format format, std::size_t buffering = 2) {
		auto ret = SwapchainRenderer{vram};
		auto renderer = Renderer::make(vram.device.device, format, vram.colourSamples);
		if (!renderer.renderPass) { return {}; }
		ret.renderer = std::move(renderer);

		static constexpr auto flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
		auto cpci = vk::CommandPoolCreateInfo(flags, vram.device.queue.family);
		ret.commandPool = vram.device.device.createCommandPoolUnique(cpci);

		ret.vram.buffering = buffering;
		auto const count = static_cast<std::uint32_t>(buffering);
		auto primaries = vram.device.device.allocateCommandBuffers({*ret.commandPool, vk::CommandBufferLevel::ePrimary, count});
		auto secondaries = vram.device.device.allocateCommandBuffers({*ret.commandPool, vk::CommandBufferLevel::eSecondary, count});
		for (std::size_t i = 0; i < buffering; ++i) {
			auto sync = FrameSync::make(vram.device.device);
			sync.cmd.primary = std::move(primaries[i]);
			sync.cmd.secondary = std::move(secondaries[i]);
			ret.frameSync.push(std::move(sync));
		}

		if (vram.colourSamples > vk::SampleCountFlagBits::e1) {
			ret.msaa = true;
			auto& image = ret.msaaImage;
			image = {{vram, "render_pass_msaa_image"}};
			image.setColour();
			image.info.info.samples = vram.colourSamples;
			image.info.info.format = format;
			VF_TRACE("vf::(internal)", trace::Type::eInfo, "Using custom MSAA render target");
		}

		return ret;
	}

	explicit operator bool() const { return static_cast<bool>(renderer.renderPass); }
	VKSync sync() const { return frameSync.get().sync(); }

	vk::CommandBuffer beginRender(VKImage acquired, Extent extent) {
		if (!renderer.renderPass) { return {}; }

		auto& sync = frameSync.get();
		vram.device.reset(*sync.drawn);
		framebuffer.colour = acquired;
		if (msaa) {
			framebuffer.colour = msaaImage.refresh(extent);
			framebuffer.resolve = acquired;
		}
		framebuffer.extent = vk::Extent2D(extent.x, extent.y);
		sync.framebuffer = renderer.makeFramebuffer(framebuffer);
		if (!sync.framebuffer) { return {}; }

		framebuffer.framebuffer = *sync.framebuffer;
		present = acquired;
		auto cmd = sync.cmd.secondary;
		auto const cbii = vk::CommandBufferInheritanceInfo(*renderer.renderPass, 0U, framebuffer);
		cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue, &cbii});
		cmd.setScissor(0, vk::Rect2D({}, framebuffer.colour.extent));

		return sync.cmd.secondary;
	}

	vk::CommandBuffer endRender() {
		if (!renderer.renderPass || !framebuffer) { return {}; }

		auto& sync = frameSync.get();
		sync.cmd.secondary.end();
		sync.cmd.primary.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

		auto images = ktl::fixed_vector<VKImage, 2>{framebuffer.colour};
		if (msaa) { images.push_back(framebuffer.resolve); }

		auto frame = Renderer::Frame{renderer, framebuffer, sync.cmd.primary};
		frame.undefToColour(images);
		frame.render(clear, {&sync.cmd.secondary, 1});
		frame.colourToPresent(present);

		sync.cmd.primary.end();
		framebuffer = {};
		present = {};

		return sync.cmd.primary;
	}

	void next() { frameSync.next(); }
};

ShaderInput::Textures makeShaderTextures(Vram const& vram) {
	auto ret = ShaderInput::Textures{};
	auto sci = samplerInfo(vram, vk::SamplerAddressMode::eClampToBorder, vk::Filter::eNearest);
	ret.sampler = vram.device.device.createSamplerUnique(sci);
	ret.white = {{vram, "white"}};
	ret.magenta = {{vram, "magenta"}};
	ret.white.setTexture(false);
	ret.magenta.setTexture(false);

	ret.white.refresh({1, 1});
	ret.magenta.refresh({1, 1});
	if (!ret.white.view || !ret.magenta.view) { return {}; }
	std::byte imageBytes[4]{};
	Bitmap::rgbaToByte(white_v, imageBytes);
	auto cb = GfxCommandBuffer(vram);
	cb.writer.write(ret.white.image.get(), std::span<std::byte const>(imageBytes), {}, vk::ImageLayout::eShaderReadOnlyOptimal);
	Bitmap::rgbaToByte(magenta_v, imageBytes);
	cb.writer.write(ret.magenta.image.get(), std::span<std::byte const>(imageBytes), {}, vk::ImageLayout::eShaderReadOnlyOptimal);

	return ret;
}
} // namespace

glm::mat4 projection(Extent const extent, glm::vec2 const nf = {-100.0f, 100.0f}) {
	if (extent.x == 0 || extent.y == 0) { return {}; }
	auto const half = glm::vec2(static_cast<float>(extent.x), static_cast<float>(extent.y)) * 0.5f;
	return glm::ortho(-half.x, half.x, -half.y, half.y, nf.x, nf.y);
}

struct VulkifyInstance::Impl {
	std::shared_ptr<UniqueGlfw> glfw{};
	UniqueWindow window{};
	VKInstance vulkan{};
	UniqueVram vram{};
	VKSurface surface{};
	SwapchainRenderer renderer{};
	FtUnique<FtLib> freetype{};

	VKSurface::Acquire acquired{};
	std::vector<vk::UniqueDescriptorSetLayout> setLayouts{};
	VIStorage vertexInput{};
	PipelineFactory pipelineFactory{};
	DescriptorSetFactory setFactory{};
	ShaderInput::Textures shaderTextures{};
	Camera camera{};
};

VulkifyInstance::VulkifyInstance(ktl::kunique_ptr<Impl> impl) noexcept : m_impl(std::move(impl)) {
	g_glfw = {m_impl->window->win, &m_impl->window->events, &m_impl->window->scancodes, &m_impl->window->fileDrops};
}

VulkifyInstance::~VulkifyInstance() {
	m_impl->vulkan.device->waitIdle();
	g_glfw = {};
	g_gamepads = {};
}

PhysicalDevice selectDevice(std::span<PhysicalDevice> devices, GpuSelector const* gpuSelector) {
	assert(!devices.empty());
	std::size_t index{};
	if (gpuSelector) {
		auto gpus = std::vector<Gpu>{};
		gpus.reserve(devices.size());
		for (auto const& device : devices) { gpus.push_back(device.gpu); }
		auto const first = gpus.data();
		auto const last = first + gpus.size();
		if (auto gpu = gpuSelector->operator()(first, last); gpu < last) { index = gpu - first; }
	}
	return std::move(devices[index]);
}

VulkifyInstance::Result VulkifyInstance::make(CreateInfo const& createInfo) {
	if (g_glfw.window) { return Error::eDuplicateInstance; }
	if (createInfo.extent.x == 0 || createInfo.extent.y == 0) { return Error::eInvalidArgument; }
	if (createInfo.instanceFlags.test(InstanceFlag::eHeadless)) { return Error::eInvalidArgument; }

	auto glfw = getOrMakeGlfw();
	if (!glfw) { return glfw.error(); }
	auto window = makeWindow(createInfo);
	if (!window) { return Error::eGlfwFailure; }

	auto freetype = FtLib::make();
	if (!freetype) { return Error::eFreetypeInitFailure; }

	auto vkinfo = VKInstance::Info{};
	vkinfo.makeSurface = [&window](vk::Instance inst) {
		auto surface = VkSurfaceKHR{};
		[[maybe_unused]] auto const res = glfwCreateWindowSurface(static_cast<VkInstance>(inst), window->win, {}, &surface);
		assert(res == VK_SUCCESS);
		return vk::SurfaceKHR(surface);
	};
	vkinfo.instanceExtensions = glfw->get()->get().instanceExtensions();
	auto builder = VKInstance::Builder::make(std::move(vkinfo));
	if (!builder) { return builder.error(); }
	if (builder->devices.empty()) { return Error::eNoVulkanSupport; }
	auto selected = selectDevice(builder->devices, createInfo.gpuSelector);
	auto instance = builder.value()(std::move(selected));
	if (!instance) { return instance.error(); }

	auto vulkan = std::move(instance.value());
	auto device = makeDevice(vulkan);
	auto vram = UniqueVram::make(*vulkan.instance, device, getSamples(createInfo.desiredAA));
	if (!vram) { return Error::eVulkanInitFailure; }
	vram.vram->ftlib = freetype.lib;

	auto impl = ktl::make_unique<Impl>(std::move(*glfw), std::move(window), std::move(vulkan), std::move(vram));
	bool const linear = createInfo.instanceFlags.test(InstanceFlag::eLinearSwapchain);
	impl->surface = VKSurface::make(device, impl->vulkan.gpu, *impl->vulkan.surface, getFramebufferSize(impl->window->win), linear);
	if (!impl->surface) { return Error::eVulkanInitFailure; }
	device.flags.assign(VKDevice::Flag::eLinearSwp, impl->surface.linear);

	auto renderer = SwapchainRenderer::make(impl->vram.vram, impl->surface.info.imageFormat);
	if (!renderer) { return Error::eVulkanInitFailure; }
	impl->renderer = std::move(renderer);
	impl->vram.vram->textureFormat = textureFormat(impl->surface.info.imageFormat);

	impl->setLayouts = makeSetLayouts(impl->surface.device.device);
	impl->vertexInput = VIStorage::make();
	auto const srr = createInfo.instanceFlags.test(InstanceFlag::eSuperSampling);
	auto sl = makeSetLayouts(impl->setLayouts);
	auto const csamples = impl->vram.vram->colourSamples;
	impl->pipelineFactory = PipelineFactory::make(impl->vram.vram->device, impl->vertexInput(), std::move(sl), csamples, srr);
	if (!impl->pipelineFactory) { return Error::eVulkanInitFailure; }

	impl->vram.vram->buffering = impl->renderer.frameSync.storage.size();
	impl->setFactory = DescriptorSetFactory::make(impl->vram.vram, impl->pipelineFactory.setLayouts);
	if (!impl->setFactory) { return Error::eVulkanInitFailure; }

	impl->shaderTextures = makeShaderTextures(impl->vram.vram);
	if (!impl->shaderTextures) { return Error::eVulkanInitFailure; }

	impl->freetype = freetype;

	return ktl::kunique_ptr<VulkifyInstance>(new VulkifyInstance(std::move(impl)));
}

Gpu const& VulkifyInstance::gpu() const { return m_impl->vulkan.gpu.gpu; }

bool VulkifyInstance::closing() const {
	auto const ret = glfwWindowShouldClose(m_impl->window->win);
	if (ret) { m_impl->vulkan.device->waitIdle(); }
	return ret;
}

Extent VulkifyInstance::framebufferExtent() const { return getFramebufferSize(m_impl->window->win); }
Extent VulkifyInstance::windowExtent() const { return getWindowSize(m_impl->window->win); }
glm::ivec2 VulkifyInstance::position() const { return getGlfwVec<int>(m_impl->window->win, &glfwGetWindowPos); }
glm::vec2 VulkifyInstance::contentScale() const { return getGlfwVec<float>(m_impl->window->win, glfwGetWindowContentScale); }
CursorMode VulkifyInstance::cursorMode() const { return castCursorMode(glfwGetInputMode(m_impl->window->win, GLFW_CURSOR)); }

glm::vec2 VulkifyInstance::cursorPosition() const {
	auto const pos = getGlfwVec<double>(m_impl->window->win, &glfwGetCursorPos);
	auto const hwin = glm::vec2(windowExtent()) * 0.5f;
	return {pos.x - hwin.x, hwin.y - pos.y};
}

MonitorList VulkifyInstance::monitors() const {
	auto ret = MonitorList{};
	int count;
	auto monitors = glfwGetMonitors(&count);
	if (count <= 0) { return {}; }
	ret.primary = makeMonitor(monitors[0]);
	ret.others.reserve(static_cast<std::size_t>(count - 1));
	for (int i = 1; i < count; ++i) { ret.others.push_back(makeMonitor(monitors[i])); }
	return ret;
}

WindowFlags VulkifyInstance::windowFlags() const {
	auto ret = WindowFlags{};
	ret.assign(WindowFlag::eBorderless, !glfwGetWindowAttrib(m_impl->window->win, GLFW_DECORATED));
	ret.assign(WindowFlag::eResizable, glfwGetWindowAttrib(m_impl->window->win, GLFW_RESIZABLE));
	ret.assign(WindowFlag::eFloating, glfwGetWindowAttrib(m_impl->window->win, GLFW_FLOATING));
	ret.assign(WindowFlag::eAutoIconify, glfwGetWindowAttrib(m_impl->window->win, GLFW_AUTO_ICONIFY));
	ret.assign(WindowFlag::eMaximized, glfwGetWindowAttrib(m_impl->window->win, GLFW_MAXIMIZED));
	return ret;
}

AntiAliasing VulkifyInstance::antiAliasing() const {
	switch (m_impl->vram.vram->colourSamples) {
	case vk::SampleCountFlagBits::e16: return AntiAliasing::e16x;
	case vk::SampleCountFlagBits::e8: return AntiAliasing::e8x;
	case vk::SampleCountFlagBits::e4: return AntiAliasing::e4x;
	case vk::SampleCountFlagBits::e2: return AntiAliasing::e2x;
	case vk::SampleCountFlagBits::e1:
	default: return AntiAliasing::eNone;
	}
}

VSync VulkifyInstance::vsync() const { return toVSync(m_impl->surface.info.presentMode); }

std::vector<Gpu> VulkifyInstance::gpuList() const { return m_impl->vulkan.availableDevices(); }

void VulkifyInstance::setPosition(glm::ivec2 xy) { glfwSetWindowPos(m_impl->window->win, xy.x, xy.y); }
void VulkifyInstance::setExtent(Extent size) { glfwSetWindowSize(m_impl->window->win, static_cast<int>(size.x), static_cast<int>(size.y)); }
void VulkifyInstance::setCursorMode(CursorMode mode) { glfwSetInputMode(m_impl->window->win, GLFW_CURSOR, cast(mode)); }

void VulkifyInstance::setIcons(std::span<Icon const> icons) {
	if (icons.empty()) {
		glfwSetWindowIcon(m_impl->window->win, 0, {});
		return;
	}
	auto vec = std::vector<GLFWimage>{};
	vec.reserve(icons.size());
	for (auto const& icon : icons) {
		auto const extent = glm::ivec2(icon.bitmap.extent);
		auto const pixels = const_cast<unsigned char*>(reinterpret_cast<unsigned char const*>(icon.bitmap.data.data()));
		vec.push_back({extent.x, extent.y, pixels});
	}
	glfwSetWindowIcon(m_impl->window->win, static_cast<int>(vec.size()), vec.data());
}

void VulkifyInstance::setWindowed(Extent extent) {
	auto const ext = glm::ivec2(extent);
	glfwSetWindowMonitor(m_impl->window->win, nullptr, 0, 0, ext.x, ext.y, 0);
}

void VulkifyInstance::setFullscreen(Monitor const& monitor, Extent resolution) {
	auto gmonitor = reinterpret_cast<GLFWmonitor*>(monitor.handle);
	auto const vmode = glfwGetVideoMode(gmonitor);
	if (!vmode) { return; }
	auto res = glm::ivec2(resolution);
	if (resolution.x == 0 || resolution.y == 0) { resolution = {vmode->width, vmode->height}; }
	glfwSetWindowMonitor(m_impl->window->win, gmonitor, 0, 0, res.x, res.y, vmode->refreshRate);
}

void VulkifyInstance::updateWindowFlags(WindowFlags set, WindowFlags unset) {
	auto updateAttrib = [&](WindowFlag flag, int attrib, bool ifSet) {
		if (set.test(flag)) { glfwSetWindowAttrib(m_impl->window->win, attrib, ifSet ? GLFW_TRUE : GLFW_FALSE); }
		if (unset.test(flag)) { glfwSetWindowAttrib(m_impl->window->win, attrib, ifSet ? GLFW_FALSE : GLFW_TRUE); }
	};
	updateAttrib(WindowFlag::eBorderless, GLFW_DECORATED, false);
	updateAttrib(WindowFlag::eFloating, GLFW_FLOATING, true);
	updateAttrib(WindowFlag::eResizable, GLFW_RESIZABLE, true);
	updateAttrib(WindowFlag::eAutoIconify, GLFW_AUTO_ICONIFY, true);
	auto const maximized = glfwGetWindowAttrib(m_impl->window->win, GLFW_MAXIMIZED);
	if (maximized) {
		if (unset.test(WindowFlag::eMaximized)) { glfwRestoreWindow(m_impl->window->win); }
	} else {
		if (set.test(WindowFlag::eMaximized)) { glfwMaximizeWindow(m_impl->window->win); }
	}
}

bool VulkifyInstance::setVSync(VSync vsync) {
	[[maybe_unused]] static constexpr std::string_view modes_v[] = {"On", "Adaptive", "TripleBuffer", "Off"};
	if (m_impl->surface.info.presentMode == fromVSync(vsync)) { return true; }
	if (!m_impl->vulkan.gpu.gpu.presentModes.test(vsync)) {
		VF_TRACEW("vf::(internal)", "Unsupported VSync mode requested [{}]", modes_v[static_cast<int>(vsync)]);
		return false;
	}
	m_impl->surface.info.presentMode = fromVSync(vsync);
	auto res = m_impl->surface.refresh(framebufferExtent());
	if (res != vk::Result::eSuccess) {
		VF_TRACE("vf::(internal)", trace::Type::eError, "Failed to create swapchain!");
		return false;
	}
	VF_TRACEI("vf::(internal)", "VSync set to [{}]", modes_v[static_cast<int>(vsync)]);
	return true;
}

Camera& VulkifyInstance::camera() { return m_impl->camera; }

Cursor VulkifyInstance::makeCursor(Icon icon) {
	auto const extent = glm::ivec2(icon.bitmap.extent);
	auto const pixels = const_cast<unsigned char*>(reinterpret_cast<unsigned char const*>(icon.bitmap.data.data()));
	auto const img = GLFWimage{extent.x, extent.y, pixels};
	auto const glfwCursor = glfwCreateCursor(&img, icon.hotspot.x, icon.hotspot.y);
	if (glfwCursor) {
		auto& cursors = m_impl->glfw->get().cursors;
		auto const ret = Cursor{++cursors.next};
		cursors.cursors.insert_or_assign(ret, glfwCursor);
		return ret;
	}
	return {};
}

void VulkifyInstance::destroyCursor(Cursor cursor) { m_impl->glfw->get().cursors.cursors.erase(cursor); }

bool VulkifyInstance::setCursor(Cursor cursor) {
	if (cursor == Cursor{}) {
		glfwSetCursor(m_impl->window->win, {});
		return true;
	}
	auto& cursors = m_impl->glfw->get().cursors;
	auto it = cursors.cursors.find(cursor);
	if (it == cursors.cursors.end()) { return false; }
	glfwSetCursor(m_impl->window->win, it->second);
	return true;
}

void VulkifyInstance::show() { glfwShowWindow(m_impl->window->win); }
void VulkifyInstance::hide() { glfwHideWindow(m_impl->window->win); }
void VulkifyInstance::close() { glfwSetWindowShouldClose(m_impl->window->win, GLFW_TRUE); }

EventQueue VulkifyInstance::poll() {
	m_impl->window->events.clear();
	m_impl->window->scancodes.clear();
	m_impl->window->fileDrops.clear();
	g_gamepads();
	glfwPollEvents();
	return {m_impl->window->events, m_impl->window->scancodes, m_impl->window->fileDrops};
}

Surface VulkifyInstance::beginPass(Rgba clear) {
	if (m_impl->acquired) {
		VF_TRACE("vf::(internal)", trace::Type::eWarn, "RenderPass already begun");
		return {};
	}
	auto const sync = m_impl->renderer.sync();
	auto const extent = getFramebufferSize(m_impl->window->win);
	m_impl->acquired = m_impl->surface.acquire(sync.draw, extent);
	if (!m_impl->acquired) { return {}; }
	auto& sr = m_impl->renderer;
	auto cmd = sr.beginRender(m_impl->acquired.image, extent);
	m_impl->vulkan.util->defer.decrement();
	m_impl->renderer.clear = clear;

	auto proj = m_impl->setFactory.postInc(0, "UBO:P");
	auto const mat_p = projection(extent);
	proj.write(0, &mat_p, sizeof(mat_p));

	auto const input = ShaderInput{proj, &m_impl->shaderTextures};
	auto const cam = RenderCam{extent, &m_impl->camera};
	auto const lwl = std::pair(m_impl->vram.vram->deviceLimits.lineWidthRange[0], m_impl->vram.vram->deviceLimits.lineWidthRange[1]);
	auto* mutex = &m_impl->vulkan.util->mutex.render;
	return RenderPass{this, &m_impl->pipelineFactory, &m_impl->setFactory, *sr.renderer.renderPass, std::move(cmd), input, cam, lwl, mutex};
}

bool VulkifyInstance::endPass() {
	if (!m_impl->acquired) { return false; }
	auto const cb = m_impl->renderer.endRender();
	if (!cb) { return false; }
	auto const sync = m_impl->renderer.sync();
	m_impl->surface.submit(cb, sync);
	m_impl->surface.present(m_impl->acquired, sync.present, getFramebufferSize(m_impl->window->win));
	m_impl->acquired = {};
	m_impl->renderer.next();
	m_impl->setFactory.next();
	m_impl->acquired = {};
	return true;
}

Vram const& VulkifyInstance::vram() const { return m_impl->vram.vram; }

HeadlessInstance::HeadlessInstance(Time autoclose) : m_autoclose(autoclose) {}

Vram const& HeadlessInstance::vram() const { return g_inactive.vram; }

// gamepad

GamepadMap Gamepad::map() {
	auto ret = GamepadMap{};
	for (int i = GLFW_JOYSTICK_1; i <= GLFW_JOYSTICK_16; ++i) { ret.map[i] = glfwJoystickIsGamepad(i) == GLFW_TRUE; }
	return ret;
}

Gamepad::operator bool() const {
	if (id < 0) { return false; }
	return glfwJoystickIsGamepad(GLFW_JOYSTICK_1 + id) == GLFW_TRUE;
}

char const* Gamepad::name() const {
	if (!*this) { return empty_cstr_v; }
	auto ret = glfwGetGamepadName(id);
	if (!ret) { ret = glfwGetJoystickName(id); }
	return ret ? ret : empty_cstr_v;
}

bool Gamepad::operator()(Button button) const {
	if (!*this) { return false; }
	auto const& buttons = g_gamepads.states[id].buttons;
	auto const index = static_cast<int>(button);
	if (index >= static_cast<int>(std::size(buttons))) { return false; }
	return buttons[index] == GLFW_PRESS;
}

float Gamepad::operator()(Axis axis) const {
	if (!*this) { return false; }
	auto const& axes = g_gamepads.states[id].axes;
	auto const index = static_cast<int>(axis);
	if (index >= static_cast<int>(std::size(axes))) { return false; }
	return axes[index];
}

std::size_t GamepadMap::count() const { return static_cast<std::size_t>(std::count(std::begin(map), std::end(map), true)); }
} // namespace vf
