#include <detail/vk_instance.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ktl/debug_trap.hpp>

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
#include <detail/window/window.hpp>
#include <functional>

#include <glm/mat4x4.hpp>
#include <vulkify/graphics/bitmap.hpp>
#include <vulkify/graphics/camera.hpp>
#include <vulkify/graphics/geometry.hpp>
#include <iostream>

#include <ttf/ft.hpp>

#include <ktl/ktl_version.hpp>

static_assert(ktl::version_v >= ktl::kversion{1, 4, 0});

namespace vf {
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

	vk::CommandBuffer beginRender(VKImage acquired) {
		if (!renderer.renderPass) { return {}; }

		auto& sync = frameSync.get();
		auto const extent = Extent{acquired.extent.width, acquired.extent.height};
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
	std::shared_ptr<UniqueWindowInstance> winInst{};
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
	g_window = {m_impl->window->win, &m_impl->window->events, &m_impl->window->scancodes, &m_impl->window->fileDrops};
}

VulkifyInstance::~VulkifyInstance() {
	m_impl->vulkan.device->waitIdle();
	g_window = {};
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
		if (auto gpu = gpuSelector->operator()(first, last); gpu < last) { index = static_cast<std::size_t>(gpu - first); }
	}
	return std::move(devices[index]);
}

VulkifyInstance::Result VulkifyInstance::make(CreateInfo const& createInfo) {
	if (g_window.window) { return Error::eDuplicateInstance; }
	if (createInfo.extent.x == 0 || createInfo.extent.y == 0) { return Error::eInvalidArgument; }
	if (createInfo.instanceFlags.test(InstanceFlag::eHeadless)) { return Error::eInvalidArgument; }

	auto winInst = getOrMakeWindowInstance();
	if (!winInst) { return winInst.error(); }
	auto window = makeWindow(createInfo, winInst->get()->get());
	if (!window) { return Error::eGlfwFailure; }

	auto freetype = FtLib::make();
	if (!freetype) { return Error::eFreetypeInitFailure; }

	auto vkinfo = VKInstance::Info{};
	vkinfo.makeSurface = [&window](vk::Instance inst) {
		auto surface = VkSurfaceKHR{};
		auto vkinst = static_cast<VkInstance>(inst);
		[[maybe_unused]] auto const res = window->makeSurface(&vkinst, &surface);
		assert(res);
		return vk::SurfaceKHR(surface);
	};
	vkinfo.instanceExtensions = Window::Instance::extensions();
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

	auto impl = ktl::make_unique<Impl>(std::move(*winInst), std::move(window), std::move(vulkan), std::move(vram));
	bool const linear = createInfo.instanceFlags.test(InstanceFlag::eLinearSwapchain);
	impl->surface = VKSurface::make(device, impl->vulkan.gpu, *impl->vulkan.surface, impl->window->framebufferSize(), linear);
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
	auto const ret = m_impl->window->closing();
	if (ret) { m_impl->vulkan.device->waitIdle(); }
	return ret;
}

Extent VulkifyInstance::framebufferExtent() const {
	auto const ret = m_impl->surface.info.imageExtent;
	if (ret.width > 0 && ret.height > 0) { return {ret.width, ret.height}; }
	return m_impl->window->framebufferSize();
}

Extent VulkifyInstance::windowExtent() const { return m_impl->window->windowSize(); }
glm::ivec2 VulkifyInstance::position() const { return m_impl->window->position(); }
glm::vec2 VulkifyInstance::contentScale() const { return m_impl->window->contentScale(); }
CursorMode VulkifyInstance::cursorMode() const { return m_impl->window->cursorMode(); }

glm::vec2 VulkifyInstance::cursorPosition() const {
	auto const pos = m_impl->window->cursorPos();
	auto const hwin = glm::vec2(windowExtent()) * 0.5f;
	return {pos.x - hwin.x, hwin.y - pos.y};
}

MonitorList VulkifyInstance::monitors() const { return m_impl->window->monitors(); }
WindowFlags VulkifyInstance::windowFlags() const { return m_impl->window->flags(); }

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
void VulkifyInstance::setPosition(glm::ivec2 xy) { m_impl->window->position(xy); }
void VulkifyInstance::setExtent(Extent size) { m_impl->window->windowSize(size); }
void VulkifyInstance::setCursorMode(CursorMode mode) { m_impl->window->cursorMode(mode); }
void VulkifyInstance::setIcons(std::span<Icon const> icons) { m_impl->window->setIcons(icons); }
void VulkifyInstance::setWindowed(Extent extent) { m_impl->window->setWindowed(extent); }
void VulkifyInstance::setFullscreen(Monitor const& monitor, Extent resolution) { m_impl->window->setFullscreen(monitor, resolution); }
void VulkifyInstance::updateWindowFlags(WindowFlags set, WindowFlags unset) { m_impl->window->update(set, unset); }

bool VulkifyInstance::setVSync(VSync vsync) {
	[[maybe_unused]] static constexpr std::string_view modes_v[] = {"On", "Adaptive", "TripleBuffer", "Off"};
	if (m_impl->surface.info.presentMode == fromVSync(vsync)) { return true; }
	if (!m_impl->vulkan.gpu.gpu.presentModes.test(vsync)) {
		VF_TRACEW("vf::(internal)", "Unsupported VSync mode requested [{}]", modes_v[static_cast<int>(vsync)]);
		return false;
	}
	auto res = m_impl->surface.refresh(m_impl->window->framebufferSize(), fromVSync(vsync));
	if (res != vk::Result::eSuccess) {
		VF_TRACE("vf::(internal)", trace::Type::eError, "Failed to create swapchain!");
		return false;
	}
	VF_TRACEI("vf::(internal)", "VSync set to [{}]", modes_v[static_cast<int>(vsync)]);
	return true;
}

Camera& VulkifyInstance::camera() { return m_impl->camera; }

Cursor VulkifyInstance::makeCursor(Icon icon) { return m_impl->window->makeCursor(icon); }
void VulkifyInstance::destroyCursor(Cursor cursor) { m_impl->window->destroyCursor(cursor); }
bool VulkifyInstance::setCursor(Cursor cursor) { return m_impl->window->setCursor(cursor); }
void VulkifyInstance::show() { m_impl->window->show(); }
void VulkifyInstance::hide() { m_impl->window->hide(); }
void VulkifyInstance::close() { m_impl->window->close(); }

EventQueue VulkifyInstance::poll() {
	m_impl->window->events.clear();
	m_impl->window->scancodes.clear();
	m_impl->window->fileDrops.clear();
	g_gamepads();
	m_impl->window->poll();
	return {m_impl->window->events, m_impl->window->scancodes, m_impl->window->fileDrops};
}

Surface VulkifyInstance::beginPass(Rgba clear) {
	if (m_impl->acquired) {
		VF_TRACE("vf::(internal)", trace::Type::eWarn, "RenderPass already begun");
		return {};
	}
	auto const sync = m_impl->renderer.sync();
	m_impl->acquired = m_impl->surface.acquire(sync.draw, m_impl->window->framebufferSize());
	auto const extent = Extent{m_impl->acquired.image.extent.width, m_impl->acquired.image.extent.height};
	if (!m_impl->acquired) { return {}; }
	auto& sr = m_impl->renderer;
	auto cmd = sr.beginRender(m_impl->acquired.image);
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
	m_impl->surface.present(m_impl->acquired, sync.present, m_impl->window->framebufferSize());
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

GamepadMap Gamepad::map() { return Window::gamepads(); }
void Gamepad::updateMappings(char const* sdlGameControllerDb) { Window::updateGamepadMappings(sdlGameControllerDb); }

Gamepad::operator bool() const {
	if (id < 0) { return false; }
	return Window::isGamepad(id);
}

char const* Gamepad::name() const {
	if (!*this) { return ""; }
	return Window::gamepadName(id);
}

bool Gamepad::operator()(Button button) const {
	if (!*this) { return false; }
	return Window::isPressed(id, button);
}

float Gamepad::operator()(Axis axis) const {
	if (!*this) { return {}; }
	return Window::value(id, axis);
}

std::size_t GamepadMap::count() const { return static_cast<std::size_t>(std::count(std::begin(map), std::end(map), true)); }
} // namespace vf
