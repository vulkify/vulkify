#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <detail/vk_instance.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/core/unique.hpp>

#include <vulkify/instance/headless_instance.hpp>
#include <vulkify/instance/vf_instance.hpp>
#include <memory>
#include <vector>

#include <detail/descriptor_set.hpp>
#include <detail/pipeline_factory.hpp>
#include <detail/renderer.hpp>
#include <detail/rotator.hpp>
#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <detail/vk_surface.hpp>

#include <detail/vram.hpp>

#include <glm/mat4x4.hpp>
#include <vulkify/graphics/bitmap.hpp>
#include <iostream>

namespace vf {
namespace {
using EventsStorage = ktl::fixed_vector<Event, Instance::max_events_v>;
using ScancodeStorage = ktl::fixed_vector<std::uint32_t, Instance::max_scancodes_v>;
using FileDropStorage = std::vector<std::string>;

struct {
	GLFWwindow* window{};
	EventsStorage* events{};
	ScancodeStorage* scancodes{};
	FileDropStorage* fileDrops{};

	bool match(GLFWwindow const* w) const { return w == window && events && scancodes && fileDrops; }
} g_glfw;

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
struct GlfwDeleter {
	void operator()(bool) const { glfwTerminate(); }
};
using UniqueGlfw = Unique<bool, GlfwDeleter>;

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
	// TODO
	// glfwSetErrorCallback([](int code, char const* szDesc) { log("GLFW Error! [{}]: {}", code, szDesc); });
	auto ret = std::make_shared<UniqueGlfw>(true);
	s_glfw = ret;
	return ret;
}

UniqueWindow makeWindow(InstanceCreateInfo const& info) {
	using Flag = InstanceCreateInfo::Flag;
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_DECORATED, info.flags.test(Flag::eBorderless) ? 0 : 1);
	glfwWindowHint(GLFW_VISIBLE, 0);
	glfwWindowHint(GLFW_MAXIMIZED, info.flags.test(Flag::eMaximized) ? 1 : 0);
	auto ret = glfwCreateWindow(int(info.extent.x), int(info.extent.y), info.title.c_str(), nullptr, nullptr);
	attachCallbacks(ret);
	return Window{ret};
}

glm::ivec2 getFramebufferSize(GLFWwindow* w) {
	int x, y;
	glfwGetFramebufferSize(w, &x, &y);
	return {x, y};
}

glm::ivec2 getWindowSize(GLFWwindow* w) {
	int x, y;
	glfwGetWindowSize(w, &x, &y);
	return {x, y};
}
} // namespace

namespace {
constexpr vk::Format textureFormat(vk::Format surface) { return Renderer::isSrgb(surface) ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm; }

VKDevice makeDevice(VKInstance const& instance) {
	auto const flags = instance.messenger ? VKDevice::Flag::eDebugMsgr : VKDevice::Flags{};
	return {instance.queue, instance.gpu.device, *instance.device, {&instance.util->defer}, &instance.util->mutex, flags};
}

vk::UniqueDescriptorSetLayout makeSetLayout(vk::Device device, std::span<vk::DescriptorSetLayoutBinding const> bindings) {
	auto info = vk::DescriptorSetLayoutCreateInfo({}, static_cast<std::uint32_t>(bindings.size()), bindings.data());
	return device.createDescriptorSetLayoutUnique(info);
}

std::vector<vk::UniqueDescriptorSetLayout> makeSetLayouts(vk::Device device) {
	using DSet = DescriptorSet;
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

vk::Format bestDepth(vk::PhysicalDevice pd) {
	static constexpr vk::Format formats[] = {vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat, vk::Format::eD24UnormS8Uint};
	static constexpr auto features = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
	for (auto const format : formats) {
		vk::FormatProperties const props = pd.getFormatProperties(format);
		if ((props.optimalTilingFeatures & features) == features) { return format; }
	}
	return vk::Format::eD16Unorm;
}

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

	VKSync sync() const { return {*draw, *present, *drawn}; }
};

struct SwapchainRenderer {
	enum Image { eColour, eDepth };
	static constexpr vk::Format colour_v = vk::Format::eR8G8B8A8Srgb;

	Vram vram{};

	Renderer renderer{};
	Rotator<FrameSync> frameSync{};
	ImageCache images[2]{};
	vk::UniqueCommandPool commandPool{};

	VKImage acquired{};
	RenderTarget target{};
	Rgba clear{};

	static SwapchainRenderer make(Vram const& vram, std::size_t buffering = 2) {
		auto ret = SwapchainRenderer{vram};
		auto const depth = bestDepth(vram.device.gpu);
		auto renderer = Renderer::make(vram.device.device, colour_v, depth);
		if (!renderer.renderPass) { return {}; }
		ret.renderer = std::move(renderer);

		static constexpr auto flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
		auto cpci = vk::CommandPoolCreateInfo(flags, vram.device.queue.family);
		ret.commandPool = vram.device.device.createCommandPoolUnique(cpci);
		auto const count = static_cast<std::uint32_t>(buffering);
		auto primaries = vram.device.device.allocateCommandBuffers({*ret.commandPool, vk::CommandBufferLevel::ePrimary, count});
		auto secondaries = vram.device.device.allocateCommandBuffers({*ret.commandPool, vk::CommandBufferLevel::eSecondary, count});
		for (std::size_t i = 0; i < buffering; ++i) {
			auto sync = FrameSync::make(vram.device.device);
			sync.cmd.primary = std::move(primaries[i]);
			sync.cmd.secondary = std::move(secondaries[i]);
			ret.frameSync.push(std::move(sync));
		}

		ret.images[eColour] = {{vram, "render_pass_colour_image"}};
		ret.images[eColour].setColour();
		ret.images[eColour].info.info.format = colour_v;

		ret.images[eDepth] = {{vram, "render_pass_depth_image"}};
		ret.images[eDepth].setDepth();
		ret.images[eDepth].info.info.format = depth;

		return ret;
	}

	explicit operator bool() const { return static_cast<bool>(renderer.renderPass); }
	VKSync sync() const { return frameSync.get().sync(); }

	vk::CommandBuffer beginRender(VKImage acquired) {
		if (!renderer.renderPass) { return {}; }

		auto const ext = vk::Extent3D(acquired.extent, 1);
		auto& sync = frameSync.get();
		vram.device.reset(*sync.drawn);
		target.colour = images[eColour].refresh(ext);
		target.depth = images[eDepth].refresh(ext);
		sync.framebuffer = renderer.makeFramebuffer(target);
		if (!sync.framebuffer) { return {}; }
		this->acquired = acquired;
		target.framebuffer = *sync.framebuffer;

		auto cmd = sync.cmd.secondary;
		auto const cbii = vk::CommandBufferInheritanceInfo(*renderer.renderPass, 0U, *sync.framebuffer);
		cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue, &cbii});
		auto const height = static_cast<float>(acquired.extent.height);
		cmd.setViewport(0, vk::Viewport({}, height, static_cast<float>(acquired.extent.width), -height));
		cmd.setScissor(0, vk::Rect2D({}, acquired.extent));

		return sync.cmd.secondary;
	}

	vk::CommandBuffer endRender() {
		if (!renderer.renderPass || !target.framebuffer) { return {}; }

		auto& sync = frameSync.get();
		sync.cmd.secondary.end();
		sync.cmd.primary.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
		renderer.render(target, clear, sync.cmd.primary, {&sync.cmd.secondary, 1});

		renderer.blit(sync.cmd.primary, target, acquired.image, vk::ImageLayout::ePresentSrcKHR);
		sync.cmd.primary.end();
		target = {};

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

	ret.white.refresh(vk::Extent3D(1, 1, 1));
	ret.magenta.refresh(vk::Extent3D(1, 1, 1));
	if (!ret.white.view || !ret.magenta.view) { return {}; }
	std::byte imageBytes[4]{};
	Bitmap::rgbaToByte(white_v, imageBytes);
	auto cmd = InstantCommand(vram.commandFactory->get());
	auto writer = ImageWriter{vram, cmd.cmd};
	writer.write(ret.white.image.get(), imageBytes, {}, {}, vk::ImageLayout::eShaderReadOnlyOptimal);
	Bitmap::rgbaToByte(magenta_v, imageBytes);
	writer.write(ret.magenta.image.get(), imageBytes, {}, {}, vk::ImageLayout::eShaderReadOnlyOptimal);
	cmd.submit();

	return ret;
}
} // namespace

namespace ubo {
constexpr View view(vk::Extent2D const extent, glm::vec2 const nf = {-100.0f, 100.0f}) {
	if (extent.width == 0 || extent.height == 0) { return {}; }
	auto const half = glm::vec2(static_cast<float>(extent.width), static_cast<float>(extent.height)) * 0.5f;
	return {glm::mat4(1.0f), glm::ortho(-half.x, half.x, -half.y, half.y, nf.x, nf.y)};
}
} // namespace ubo

struct VulkifyInstance::Impl {
	std::shared_ptr<UniqueGlfw> glfw{};
	UniqueWindow window{};
	VKInstance vulkan{};
	UniqueVram vram{};
	VKSurface surface{};
	SwapchainRenderer renderer{};
	Gpu gpu{};

	std::optional<VKSurface::Acquire> acquired{};
	std::vector<vk::UniqueDescriptorSetLayout> setLayouts{};
	VIStorage vertexInput{};
	PipelineFactory pipelineFactory{};
	DescriptorPool descriptorPool{};
	ShaderInput::Textures shaderTextures{};
};

Gpu makeGPU(VKInstance const& vulkan) {
	auto ret = Gpu{};
	ret.name = vulkan.gpu.properties.deviceName.data();
	switch (vulkan.gpu.properties.deviceType) {
	case vk::PhysicalDeviceType::eCpu: ret.type = Gpu::Type::eCpu; break;
	case vk::PhysicalDeviceType::eDiscreteGpu: ret.type = Gpu::Type::eDiscrete; break;
	case vk::PhysicalDeviceType::eIntegratedGpu: ret.type = Gpu::Type::eIntegrated; break;
	case vk::PhysicalDeviceType::eVirtualGpu: ret.type = Gpu::Type::eVirtual; break;
	case vk::PhysicalDeviceType::eOther: ret.type = Gpu::Type::eOther; break;
	default: break;
	}
	return ret;
}

VulkifyInstance::VulkifyInstance(ktl::kunique_ptr<Impl> impl) noexcept : m_impl(std::move(impl)) {
	g_glfw = {m_impl->window->win, &m_impl->window->events, &m_impl->window->scancodes, &m_impl->window->fileDrops};
}
VulkifyInstance::VulkifyInstance(VulkifyInstance&&) noexcept = default;
VulkifyInstance& VulkifyInstance::operator=(VulkifyInstance&&) noexcept = default;
VulkifyInstance::~VulkifyInstance() noexcept {
	m_impl->vulkan.device->waitIdle();
	g_glfw = {};
}

VulkifyInstance::Result VulkifyInstance::make(CreateInfo const& createInfo) {
	if (createInfo.extent.x == 0 || createInfo.extent.y == 0) { return Error::eInvalidArgument; }
	if (createInfo.flags.test(InstanceCreateInfo::Flag::eHeadless)) { return Error::eInvalidArgument; }

	auto glfw = getOrMakeGlfw();
	if (!glfw) { return glfw.error(); }
	auto window = makeWindow(createInfo);
	if (!window) { return Error::eGlfwFailure; }

	auto vulkan = VKInstance::make([&window](vk::Instance inst) {
		auto surface = VkSurfaceKHR{};
		[[maybe_unused]] auto const res = glfwCreateWindowSurface(static_cast<VkInstance>(inst), window->win, {}, &surface);
		assert(res == VK_SUCCESS);
		return vk::SurfaceKHR(surface);
	});
	if (!vulkan) { return vulkan.error(); }
	auto device = makeDevice(*vulkan);
	auto vram = UniqueVram::make(*vulkan->instance, device);
	if (!vram) { return Error::eVulkanInitFailure; }

	auto impl = ktl::make_unique<Impl>(std::move(*glfw), std::move(window), std::move(*vulkan), std::move(vram));
	bool const linear = createInfo.flags.test(InstanceCreateInfo::Flag::eLinearSwapchain);
	impl->surface = VKSurface::make(device, impl->vulkan.gpu, *impl->vulkan.surface, getFramebufferSize(impl->window->win), linear);
	if (!impl->surface) { return Error::eVulkanInitFailure; }
	device.flags.assign(VKDevice::Flag::eLinearSwp, impl->surface.linear);

	auto presenter = SwapchainRenderer::make(impl->vram.vram);
	if (!presenter) { return Error::eVulkanInitFailure; }
	impl->renderer = std::move(presenter);
	impl->vram.vram->textureFormat = textureFormat(impl->surface.info.imageFormat);

	impl->setLayouts = makeSetLayouts(impl->surface.device.device);
	impl->vertexInput = VIStorage::make();
	impl->pipelineFactory = PipelineFactory::make(impl->surface.device, impl->vertexInput(), makeSetLayouts(impl->setLayouts));
	if (!impl->pipelineFactory) { return Error::eVulkanInitFailure; }

	auto const buffering = impl->renderer.frameSync.storage.size();
	impl->descriptorPool = DescriptorPool::make(impl->vram.vram, impl->pipelineFactory.setLayouts, buffering);
	if (!impl->descriptorPool) { return Error::eVulkanInitFailure; }

	impl->shaderTextures = makeShaderTextures(impl->vram.vram);
	if (!impl->shaderTextures) { return Error::eVulkanInitFailure; }

	impl->gpu = makeGPU(*vulkan);

	return ktl::kunique_ptr<VulkifyInstance>(new VulkifyInstance(std::move(impl)));
}

Gpu const& VulkifyInstance::gpu() const { return m_impl->gpu; }
glm::ivec2 VulkifyInstance::framebufferSize() const { return getFramebufferSize(m_impl->window->win); }
glm::ivec2 VulkifyInstance::windowSize() const { return getWindowSize(m_impl->window->win); }

bool VulkifyInstance::closing() const {
	auto const ret = glfwWindowShouldClose(m_impl->window->win);
	if (ret) { m_impl->vulkan.device->waitIdle(); }
	return ret;
}

void VulkifyInstance::show() { glfwShowWindow(m_impl->window->win); }
void VulkifyInstance::hide() { glfwHideWindow(m_impl->window->win); }
void VulkifyInstance::close() { glfwSetWindowShouldClose(m_impl->window->win, GLFW_TRUE); }

Instance::Poll VulkifyInstance::poll() {
	m_impl->window->events.clear();
	m_impl->window->scancodes.clear();
	m_impl->window->fileDrops.clear();
	glfwPollEvents();
	return {m_impl->window->events, m_impl->window->scancodes, m_impl->window->fileDrops};
}

Surface VulkifyInstance::beginPass() {
	if (m_impl->acquired) {
		VF_TRACE("RenderPass already begun");
		return {};
	}
	auto const sync = m_impl->renderer.sync();
	m_impl->acquired = m_impl->surface.acquire(sync.draw, framebufferSize());
	if (!m_impl->acquired) { return {}; }
	auto& r = m_impl->renderer;
	auto cmd = r.beginRender(m_impl->acquired->image);
	m_impl->vulkan.util->defer.decrement();

	auto view = m_impl->descriptorPool.get(0, 0, "uniform:View");
	view.write(0, ubo::view(m_impl->acquired->image.extent));

	auto const input = ShaderInput{view, &m_impl->shaderTextures};
	return RenderPass{this, &m_impl->pipelineFactory, &m_impl->descriptorPool, *r.renderer.renderPass, std::move(cmd), input, &r.clear};
}

bool VulkifyInstance::endPass() {
	if (!m_impl->acquired) { return false; }
	auto const cb = m_impl->renderer.endRender();
	if (!cb) { return false; }
	auto const sync = m_impl->renderer.sync();
	m_impl->surface.submit(cb, sync);
	auto const ret = m_impl->surface.present(*m_impl->acquired, sync.present, framebufferSize());
	m_impl->acquired.reset();
	m_impl->renderer.next();
	m_impl->descriptorPool.next();
	return ret.has_value();
}

Vram const& VulkifyInstance::vram() const { return m_impl->vram.vram; }

HeadlessInstance::HeadlessInstance() : m_thread(ktl::kthread([this](ktl::kthread::stop_t stop) { run(stop); })) {}

void HeadlessInstance::run(ktl::kthread::stop_t stop) {
	while (!stop.stop_requested()) {
		auto line = std::string{};
		std::getline(std::cin, line);
		if (line[0] == 'q' || line[0] == 'Q') {
			m_closing = true;
			m_thread.request_stop();
		}
		ktl::kthread::yield();
	}
}

Vram const& HeadlessInstance::vram() const { return g_inactive.vram; }
} // namespace vf
