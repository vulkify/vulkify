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
#include <detail/trace.hpp>
#include <detail/window/window.hpp>
#include <functional>

#include <glm/mat4x4.hpp>
#include <vulkify/graphics/bitmap.hpp>
#include <vulkify/graphics/camera.hpp>
#include <vulkify/graphics/geometry.hpp>
#include <iostream>

#include <ttf/ft.hpp>

#include <detail/gfx_allocations.hpp>
#include <detail/gfx_command_buffer.hpp>
#include <detail/gfx_device.hpp>
#include <detail/vulkan_instance.hpp>
#include <detail/vulkan_swapchain.hpp>

#include <ktl/ktl_version.hpp>

static_assert(ktl::version_v >= ktl::kversion{1, 4, 3});

namespace vf {
namespace {
PhysicalDevice select_device(std::span<PhysicalDevice> devices, GpuSelector const* gpu_selector) {
	assert(!devices.empty());
	std::size_t index{};
	if (gpu_selector) {
		auto gpus = std::vector<Gpu>{};
		gpus.reserve(devices.size());
		for (auto const& device : devices) { gpus.push_back(device.gpu); }
		auto const first = gpus.data();
		auto const last = first + gpus.size();
		if (auto gpu = gpu_selector->operator()(first, last); gpu < last) { index = static_cast<std::size_t>(gpu - first); }
	}
	return std::move(devices[index]);
}

vk::PresentModeKHR select_mode(GpuInfo const& gpu, std::span<VSync const> desired) {
	for (auto const vsync : desired) {
		if (gpu.gpu.present_modes.test(vsync)) { return from_vsync(vsync); }
	}
	return vk::PresentModeKHR::eFifo;
}

constexpr int get_samples(AntiAliasing aa) {
	switch (aa) {
	case AntiAliasing::e16x: return 16;
	case AntiAliasing::e8x: return 8;
	case AntiAliasing::e4x: return 4;
	case AntiAliasing::e2x: return 2;
	case AntiAliasing::eNone:
	default: return 1;
	}
}

glm::mat4 projection(Extent const extent, glm::vec2 const nf = {-100.0f, 100.0f}) {
	if (extent.x == 0 || extent.y == 0) { return {}; }
	auto const half = glm::vec2(static_cast<float>(extent.x), static_cast<float>(extent.y)) * 0.5f;
	return glm::ortho(-half.x, half.x, -half.y, half.y, nf.x, nf.y);
}

constexpr vk::Format texture_format(vk::Format surface) { return Renderer::isSrgb(surface) ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm; }

vk::UniqueDescriptorSetLayout make_set_layout(vk::Device device, std::span<vk::DescriptorSetLayoutBinding const> bindings) {
	auto info = vk::DescriptorSetLayoutCreateInfo({}, static_cast<std::uint32_t>(bindings.size()), bindings.data());
	return device.createDescriptorSetLayoutUnique(info);
}

std::vector<vk::UniqueDescriptorSetLayout> make_set_layouts(vk::Device device) {
	using DSet = SetWriter;
	auto ret = std::vector<vk::UniqueDescriptorSetLayout>{};
	auto add_set = [&](vk::ShaderStageFlags bufferStages) {
		auto b0 = vk::DescriptorSetLayoutBinding(0, DSet::buffer_layouts_v[0].type, 1, bufferStages);
		auto b1 = vk::DescriptorSetLayoutBinding(1, DSet::buffer_layouts_v[1].type, 1, bufferStages);
		auto b2 = vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment);
		vk::DescriptorSetLayoutBinding const binds[] = {b0, b1, b2};
		ret.push_back(make_set_layout(device, binds));
	};

	// set 0: scene data
	{
		// binding 0: matrices
		auto b0 = vk::DescriptorSetLayoutBinding(0, DSet::buffer_layouts_v[0].type, 1, vk::ShaderStageFlagBits::eVertex);
		ret.push_back(make_set_layout(device, {&b0, 1}));
	}
	// set 1: object data
	add_set(vk::ShaderStageFlagBits::eVertex);
	// set 2: custom
	add_set(vk::ShaderStageFlagBits::eFragment);
	return ret;
}

std::vector<vk::DescriptorSetLayout> make_set_layouts(std::span<vk::UniqueDescriptorSetLayout const> layouts) {
	auto ret = std::vector<vk::DescriptorSetLayout>{};
	ret.reserve(layouts.size());
	for (auto const& layout : layouts) { ret.push_back(*layout); }
	return ret;
}

[[maybe_unused]] constexpr std::string_view vsync_modes_v[] = {"On", "Adaptive", "TripleBuffer", "Off"};
} // namespace

namespace {
struct VertexInputStorage {
	std::vector<vk::VertexInputBindingDescription> bindings{};
	std::vector<vk::VertexInputAttributeDescription> attributes{};

	VertexInput operator()() const { return {bindings, attributes}; }

	static VertexInputStorage make() {
		auto ret = VertexInputStorage{};
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

	SwapchainSync sync() const { return {*draw, *present, *drawn}; }
};

struct SwapchainRenderer {
	GfxDevice const* device{};

	Renderer renderer{};
	Rotator<FrameSync> frame_sync{};
	ImageCache msaa_image{};
	ImageCache depth_image{};
	vk::UniqueCommandPool command_pool{};

	Framebuffer framebuffer{};
	ImageView present{};
	Rgba clear{};
	bool msaa{};

	static SwapchainRenderer make(GfxDevice const* device, vk::Format format) {
		assert(device);
		auto ret = SwapchainRenderer{device};
		auto renderer = Renderer::make(device->device.device, format, device->colour_samples);
		if (!renderer.render_pass) { return {}; }
		ret.renderer = std::move(renderer);

		static constexpr auto flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer | vk::CommandPoolCreateFlagBits::eTransient;
		auto cpci = vk::CommandPoolCreateInfo(flags, device->device.queue.family);
		ret.command_pool = device->device.device.createCommandPoolUnique(cpci);

		auto const count = static_cast<std::uint32_t>(device->buffering);
		auto primaries = device->device.device.allocateCommandBuffers({*ret.command_pool, vk::CommandBufferLevel::ePrimary, count});
		auto secondaries = device->device.device.allocateCommandBuffers({*ret.command_pool, vk::CommandBufferLevel::eSecondary, count});
		for (std::size_t i = 0; i < device->buffering; ++i) {
			auto sync = FrameSync::make(device->device.device);
			sync.cmd.primary = std::move(primaries[i]);
			sync.cmd.secondary = std::move(secondaries[i]);
			ret.frame_sync.push(std::move(sync));
		}

		{
			auto& image = ret.depth_image;
			image = {.device = device};
			image.set_depth(false);
			image.info.info.samples = device->colour_samples;
			image.info.info.format = vk::Format::eD16Unorm;
		}

		if (device->colour_samples > vk::SampleCountFlagBits::e1) {
			ret.msaa = true;
			auto& image = ret.msaa_image;
			image = {.device = device};
			image.set_colour();
			image.info.info.samples = device->colour_samples;
			image.info.info.format = format;
			VF_TRACE("vf::(internal)", trace::Type::eInfo, "Using custom MSAA render target");
		}

		return ret;
	}

	explicit operator bool() const { return static_cast<bool>(renderer.render_pass); }
	SwapchainSync sync() const { return frame_sync.get().sync(); }

	vk::CommandBuffer begin_render(ImageView acquired) {
		if (!renderer.render_pass) { return {}; }

		auto& sync = frame_sync.get();
		auto const extent = Extent{acquired.extent.width, acquired.extent.height};
		device->device.reset(*sync.drawn);
		framebuffer.colour = acquired;
		framebuffer.depth = depth_image.refresh(extent);
		if (msaa) {
			auto refreshed = msaa_image.refresh(extent);
			framebuffer.colour = refreshed;
			framebuffer.resolve = acquired;
		}
		framebuffer.extent = vk::Extent2D(extent.x, extent.y);
		sync.framebuffer = renderer.make_framebuffer(framebuffer);
		if (!sync.framebuffer) { return {}; }

		framebuffer.framebuffer = *sync.framebuffer;
		present = acquired;
		auto cmd = sync.cmd.secondary;
		auto const cbii = vk::CommandBufferInheritanceInfo(*renderer.render_pass, 0U, framebuffer);
		cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit | vk::CommandBufferUsageFlagBits::eRenderPassContinue, &cbii});
		cmd.setScissor(0, vk::Rect2D({}, framebuffer.colour.extent));

		return sync.cmd.secondary;
	}

	vk::CommandBuffer end_render() {
		if (!renderer.render_pass || !framebuffer) { return {}; }

		auto& sync = frame_sync.get();
		sync.cmd.secondary.end();
		sync.cmd.primary.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

		auto images = ktl::fixed_vector<ImageView, 2>{framebuffer.colour};
		if (msaa) { images.push_back(framebuffer.resolve); }

		auto frame = Renderer::Frame{renderer, framebuffer, sync.cmd.primary};
		frame.undef_to_depth(framebuffer.depth);
		frame.undef_to_colour(images);
		frame.render(clear, {&sync.cmd.secondary, 1});
		frame.colour_to_present({present.image, present.view, present.extent});

		sync.cmd.primary.end();
		framebuffer = {};
		present = {};

		return sync.cmd.primary;
	}

	void next() { frame_sync.next(); }
};

ShaderInput::Textures make_shader_textures(GfxDevice const* device) {
	assert(device);
	auto ret = ShaderInput::Textures{};
	auto sci = device->sampler_info(vk::SamplerAddressMode::eClampToBorder, vk::Filter::eNearest);
	ret.sampler = device->device.device.createSamplerUnique(sci);
	ret.white = {.device = device};
	ret.magenta = {.device = device};
	ret.white.set_texture(false);
	ret.magenta.set_texture(false);

	ret.white.refresh({1, 1});
	ret.magenta.refresh({1, 1});
	if (!ret.white.view || !ret.magenta.view) { return {}; }
	std::byte imageBytes[4]{};
	Bitmap::rgba_to_byte(white_v, imageBytes);
	auto cb = GfxCommandBuffer(device);
	cb.writer.write(ret.white.image.get(), std::span<std::byte const>(imageBytes), {}, vk::ImageLayout::eShaderReadOnlyOptimal);
	Bitmap::rgba_to_byte(magenta_v, imageBytes);
	cb.writer.write(ret.magenta.image.get(), std::span<std::byte const>(imageBytes), {}, vk::ImageLayout::eShaderReadOnlyOptimal);

	return ret;
}
} // namespace

struct VulkifyInstance::Impl {
	std::shared_ptr<UniqueWindowInstance> win_inst{};
	UniqueWindow window{};
	VulkanInstance vulkan{};
	UniqueGfxDevice device{};
	VulkanSwapchain swapchain{};
	SwapchainRenderer renderer{};
	FtUnique<FtLib> freetype{};

	VulkanSwapchain::Acquire acquired{};
	std::vector<vk::UniqueDescriptorSetLayout> set_layouts{};
	VertexInputStorage vertex_input{};
	PipelineFactory pipeline_factory{};
	DescriptorSetFactory set_factory{};
	ShaderInput::Textures shader_textures{};
	Camera camera{};
	RenderPass render_pass{};
};

VulkifyInstance::Result VulkifyInstance::make(CreateInfo const& create_info) {
	if (g_window.window) { return Error::eDuplicateInstance; }
	if (create_info.extent.x == 0 || create_info.extent.y == 0) { return Error::eInvalidArgument; }
	if (create_info.instance_flags.test(InstanceFlag::eHeadless)) { return Error::eInvalidArgument; }

	auto win_inst = get_or_make_window_instance();
	if (!win_inst) { return win_inst.error(); }
	auto window = make_window(create_info, win_inst->get()->get());
	if (!window) { return Error::eGlfwFailure; }

	auto freetype = FtUnique<FtLib>{FtLib::make()};
	if (!freetype) { return Error::eFreetypeInitFailure; }

	auto vkinfo = VulkanInstance::Info{};
	vkinfo.make_surface = [&window](vk::Instance inst) {
		auto surface = VkSurfaceKHR{};
		auto vkinst = static_cast<VkInstance>(inst);
		[[maybe_unused]] auto const res = window->make_surface(&vkinst, &surface);
		assert(res);
		return vk::SurfaceKHR(surface);
	};
	vkinfo.instance_extensions = Window::Instance::extensions();
	vkinfo.desired_vsyncs = create_info.desired_vsyncs;
	auto builder = VulkanInstance::Builder::make(std::move(vkinfo));
	if (!builder) { return builder.error(); }
	if (builder->devices.empty()) { return Error::eNoVulkanSupport; }
	auto selected = select_device(builder->devices, create_info.gpu_selector);
	auto vulkan = builder.value()(std::move(selected));
	if (!vulkan) { return vulkan.error(); }

	auto impl = ktl::make_unique<Impl>(Impl{std::move(*win_inst), std::move(window), std::move(*vulkan)});
	{
		impl->device = UniqueGfxDevice::make(impl->vulkan, freetype->lib, get_samples(create_info.desired_aa));
		if (!impl->device) { return Error::eVulkanInitFailure; }
		impl->device.device->buffering = 2;
		impl->device.device->default_z_order = create_info.default_z_order;
	}
	{
		bool const linear = create_info.instance_flags.test(InstanceFlag::eLinearSwapchain);
		auto const extent = impl->window->framebuffer_size();
		auto const mode = select_mode(impl->vulkan.gpu, create_info.desired_vsyncs);
		VF_TRACEI("vf::(internal)", "VSync set to [{}]", vsync_modes_v[static_cast<int>(to_vsync(mode))]);
		impl->swapchain = VulkanSwapchain::make(&impl->device.device.get(), impl->vulkan.gpu.formats, *impl->vulkan.surface, mode, extent, linear);
		if (!impl->swapchain) { return Error::eVulkanInitFailure; }
		impl->device.device->device.flags.assign(VulkanDevice::Flag::eLinearSwp, impl->swapchain.linear);
	}
	{
		auto renderer = SwapchainRenderer::make(&impl->device.device.get(), impl->swapchain.info.imageFormat);
		if (!renderer) { return Error::eVulkanInitFailure; }
		impl->renderer = std::move(renderer);
		impl->device.device->texture_format = texture_format(impl->swapchain.info.imageFormat);
		impl->device.device->buffering = impl->renderer.frame_sync.storage.size();
	}
	{
		impl->set_layouts = make_set_layouts(*impl->vulkan.device);
		impl->vertex_input = VertexInputStorage::make();
		auto const srr = create_info.instance_flags.test(InstanceFlag::eSuperSampling);
		auto sl = make_set_layouts(impl->set_layouts);
		auto const csamples = impl->device.device->colour_samples;
		impl->pipeline_factory = PipelineFactory::make(impl->device.device->device, impl->vertex_input(), std::move(sl), csamples, srr);
		if (!impl->pipeline_factory) { return Error::eVulkanInitFailure; }
	}

	{
		impl->set_factory = DescriptorSetFactory::make(impl->device.device, impl->pipeline_factory.set_layouts);
		if (!impl->set_factory) { return Error::eVulkanInitFailure; }

		impl->shader_textures = make_shader_textures(&impl->device.device.get());
		if (!impl->shader_textures) { return Error::eVulkanInitFailure; }
	}

	impl->freetype = std::move(freetype);

	return ktl::kunique_ptr<VulkifyInstance>(new VulkifyInstance(std::move(impl)));
}

VulkifyInstance::VulkifyInstance(ktl::kunique_ptr<Impl> impl) noexcept : m_impl(std::move(impl)) {
	g_window = {m_impl->window->win, &m_impl->window->events, &m_impl->window->scancodes, &m_impl->window->file_drops};
}

VulkifyInstance::~VulkifyInstance() {
	m_impl->vulkan.device->waitIdle();
	g_window = {};
	g_gamepads = {};
}

Gpu const& VulkifyInstance::gpu() const { return m_impl->vulkan.gpu.gpu; }

bool VulkifyInstance::closing() const {
	auto const ret = m_impl->window->closing();
	if (ret) { m_impl->vulkan.device->waitIdle(); }
	return ret;
}

Extent VulkifyInstance::framebuffer_extent() const {
	auto const ret = m_impl->swapchain.info.imageExtent;
	if (ret.width > 0 && ret.height > 0) { return {ret.width, ret.height}; }
	return m_impl->window->framebuffer_size();
}

Extent VulkifyInstance::window_extent() const { return m_impl->window->window_size(); }
glm::ivec2 VulkifyInstance::position() const { return m_impl->window->position(); }
glm::vec2 VulkifyInstance::content_scale() const { return m_impl->window->content_scale(); }
CursorMode VulkifyInstance::cursor_mode() const { return m_impl->window->cursor_mode(); }

glm::vec2 VulkifyInstance::cursor_position() const {
	auto const pos = m_impl->window->cursor_position();
	auto const hwin = glm::vec2(window_extent()) * 0.5f;
	return {pos.x - hwin.x, hwin.y - pos.y};
}

MonitorList VulkifyInstance::monitors() const { return m_impl->window->monitors(); }
WindowFlags VulkifyInstance::window_flags() const { return m_impl->window->flags(); }

AntiAliasing VulkifyInstance::anti_aliasing() const {
	switch (m_impl->device.device->colour_samples) {
	case vk::SampleCountFlagBits::e16: return AntiAliasing::e16x;
	case vk::SampleCountFlagBits::e8: return AntiAliasing::e8x;
	case vk::SampleCountFlagBits::e4: return AntiAliasing::e4x;
	case vk::SampleCountFlagBits::e2: return AntiAliasing::e2x;
	case vk::SampleCountFlagBits::e1:
	default: return AntiAliasing::eNone;
	}
}

VSync VulkifyInstance::vsync() const { return to_vsync(m_impl->swapchain.info.presentMode); }
std::vector<Gpu> VulkifyInstance::gpu_list() const { return m_impl->vulkan.available_devices(); }
ZOrder VulkifyInstance::default_z_order() const { return m_impl->device.device->default_z_order; }
void VulkifyInstance::set_position(glm::ivec2 xy) { m_impl->window->position(xy); }
void VulkifyInstance::set_extent(Extent size) { m_impl->window->set_window_size(size); }
void VulkifyInstance::set_cursor_mode(CursorMode mode) { m_impl->window->set_cursor_mode(mode); }
void VulkifyInstance::set_icons(std::span<Icon const> icons) { m_impl->window->set_icons(icons); }
void VulkifyInstance::set_windowed(Extent extent) { m_impl->window->set_windowed(extent); }
void VulkifyInstance::set_fullscreen(Monitor const& monitor, Extent resolution) { m_impl->window->set_fullscreen(monitor, resolution); }
void VulkifyInstance::update_window_flags(WindowFlags set, WindowFlags unset) { m_impl->window->update(set, unset); }
Camera& VulkifyInstance::camera() { return m_impl->camera; }
Cursor VulkifyInstance::make_cursor(Icon icon) { return m_impl->window->make_cursor(icon); }
void VulkifyInstance::destroy_cursor(Cursor cursor) { m_impl->window->destroy_cursor(cursor); }
bool VulkifyInstance::set_cursor(Cursor cursor) { return m_impl->window->set_cursor(cursor); }
void VulkifyInstance::show() { m_impl->window->show(); }
void VulkifyInstance::hide() { m_impl->window->hide(); }
void VulkifyInstance::close() { m_impl->window->close(); }
void VulkifyInstance::lock_aspect_ratio(bool lock) { m_impl->window->lock_aspect_ratio(lock); }

EventQueue VulkifyInstance::poll() {
	m_impl->window->events.clear();
	m_impl->window->scancodes.clear();
	m_impl->window->file_drops.clear();
	g_gamepads();
	m_impl->window->poll();
	return {m_impl->window->events, m_impl->window->scancodes, m_impl->window->file_drops};
}

vf::Surface VulkifyInstance::begin_pass(Rgba clear) {
	if (m_impl->acquired) {
		VF_TRACE("vf::(internal)", trace::Type::eWarn, "RenderPass already begun");
		return {};
	}

	auto const sync = m_impl->renderer.sync();
	m_impl->acquired = m_impl->swapchain.acquire(sync.draw, m_impl->window->framebuffer_size());
	auto const extent = Extent{m_impl->acquired.image.extent.width, m_impl->acquired.image.extent.height};
	if (!m_impl->acquired) { return {}; }
	auto& sr = m_impl->renderer;
	auto cmd = sr.begin_render(m_impl->acquired.image);
	m_impl->vulkan.util->defer.decrement();
	m_impl->renderer.clear = clear;

	auto proj = m_impl->set_factory.post_increment(0);
	auto const mat_p = projection(extent);
	proj.write(0, &mat_p, sizeof(mat_p));

	auto const input = ShaderInput{proj, &m_impl->shader_textures};
	auto const cam = RenderCam{extent, &m_impl->camera};
	auto const lwl = std::pair(m_impl->device.device->device_limits->lineWidthRange[0], m_impl->device.device->device_limits->lineWidthRange[1]);
	auto* mutex = &m_impl->vulkan.util->mutex.render;
	auto const& device = m_impl->device.device.get();
	m_impl->render_pass =
		RenderPass{this, &device, &m_impl->pipeline_factory, &m_impl->set_factory, *sr.renderer.render_pass, std::move(cmd), input, cam, lwl, mutex};
	return Surface{&m_impl->render_pass};
}

bool VulkifyInstance::end_pass() {
	if (!m_impl->acquired) { return false; }
	auto const cb = m_impl->renderer.end_render();
	if (!cb) { return false; }
	auto const sync = m_impl->renderer.sync();
	m_impl->swapchain.submit(cb, sync);
	m_impl->swapchain.present(m_impl->acquired, sync.present, m_impl->window->framebuffer_size());
	m_impl->acquired = {};
	m_impl->renderer.next();
	m_impl->set_factory.next();
	m_impl->acquired = {};
	return true;
}
// gamepad

GamepadMap Gamepad::map() { return Window::gamepads(); }
void Gamepad::update_mappings(char const* sdl_game_controller_db) { Window::update_gamepad_mappings(sdl_game_controller_db); }

Gamepad::operator bool() const {
	if (id < 0) { return false; }
	return Window::is_gamepad(id);
}

char const* Gamepad::name() const {
	if (!*this) { return ""; }
	return Window::gamepad_name(id);
}

bool Gamepad::operator()(Button button) const {
	if (!*this) { return false; }
	return Window::is_pressed(id, button);
}

float Gamepad::operator()(Axis axis) const {
	if (!*this) { return {}; }
	return Window::value(id, axis);
}

std::size_t GamepadMap::count() const { return static_cast<std::size_t>(std::count(std::begin(map), std::end(map), true)); }

GfxDevice const& VulkifyInstance::gfx_device() const { return m_impl->device.device; }

HeadlessInstance::HeadlessInstance(Time autoclose) : m_autoclose(autoclose) {}

GfxDevice const& HeadlessInstance::gfx_device() const {
	static auto const s_inactive = GfxDevice{};
	return s_inactive;
}
} // namespace vf
