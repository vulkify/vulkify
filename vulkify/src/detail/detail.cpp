#include <detail/command_pool.hpp>
#include <detail/command_pool2.hpp>
#include <detail/gfx_buffer_image.hpp>
#include <detail/gfx_command_buffer.hpp>
#include <detail/gfx_device.hpp>
#include <detail/trace.hpp>
#include <detail/vulkan_instance.hpp>
#include <ktl/enumerate.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/vulkify_version.hpp>
#include <algorithm>
#include <atomic>
#include <unordered_set>

namespace vf {
using namespace std::string_view_literals;

void DeferQueue::decrement() {
	std::erase_if(entries, [this](Entry& e) {
		if (--e->delay <= 0) {
			expired.push_back(std::move(e));
			return true;
		}
		return false;
	});
	expired.clear();
}

namespace refactor {
/// Device
VulkanDevice VulkanDevice::make(VulkanInstance const& instance) {
	assert(instance.util);
	return VulkanDevice{
		.queue = instance.queue,
		.gpu = instance.gpu.device,
		.device = *instance.device,
		.queue_mutex = &instance.util->mutex.queue,
		.limits = &instance.util->device_limits,
		.flags = instance.messenger ? Flag::eDebugMsgr : Flags{},
	};
}

void VulkanDevice::wait(vk::Fence fence, std::uint64_t wait) const {
	if (fence) {
		auto res = device.waitForFences(1, &fence, true, static_cast<std::uint64_t>(wait));
		if (res != vk::Result::eSuccess) { VF_TRACE("vf::(internal)", trace::Type::eError, "Fence wait failure!"); }
	}
}

void VulkanDevice::reset(vk::Fence fence, std::uint64_t wait) const {
	if (wait > 0 && busy(fence)) { this->wait(fence, wait); }
	auto res = device.resetFences(1, &fence);
	if (res != vk::Result::eSuccess) { VF_TRACE("vf::(internal)", trace::Type::eError, "Fence reset failure!"); }
}

vk::UniqueImageView VulkanDevice::make_image_view(vk::Image const image, vk::Format const format, vk::ImageAspectFlags aspects) const {
	vk::ImageViewCreateInfo info;
	info.viewType = vk::ImageViewType::e2D;
	info.format = format;
	info.components.r = info.components.g = info.components.b = info.components.a = vk::ComponentSwizzle::eIdentity;
	info.subresourceRange = {aspects, 0, 1, 0, 1};
	info.image = image;
	return device.createImageViewUnique(info);
}
/// /Device

/// Instance
namespace {
constexpr auto validation_layer_v = "VK_LAYER_KHRONOS_validation"sv;

vk::UniqueInstance make_instance(std::vector<char const*> extensions, bool& out_validation) {
	if (out_validation) {
		auto const availLayers = vk::enumerateInstanceLayerProperties();
		auto layerSearch = [](vk::LayerProperties const& lp) { return lp.layerName == validation_layer_v; };
		if (std::find_if(availLayers.begin(), availLayers.end(), layerSearch) != availLayers.end()) {
			VF_TRACE("vf::(internal)", trace::Type::eInfo, "Requesting Vulkan validation layers");
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		} else {
			VF_TRACE("vf::(internal)", trace::Type::eWarn, "VK_LAYER_KHRONOS_validation not found");
			out_validation = false;
		}
	}

	auto const version = VK_MAKE_VERSION(version_v.major, version_v.minor, version_v.patch);
	auto const ai = vk::ApplicationInfo("vulkify", version, "vulkify", version, VK_API_VERSION_1_1);
	auto ici = vk::InstanceCreateInfo{};
	ici.pApplicationInfo = &ai;
#if defined(VULKIFY_VK_PORTABILITY)
	ici.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif
	ici.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
	ici.ppEnabledExtensionNames = extensions.data();
	if (out_validation) {
		static char const* layers[] = {validation_layer_v.data()};
		ici.enabledLayerCount = 1;
		ici.ppEnabledLayerNames = layers;
	}
	return vk::createInstanceUnique(ici);
}

vk::UniqueDebugUtilsMessengerEXT make_debug_messenger(vk::Instance instance) {
	auto validationCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
								 VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void*) -> vk::Bool32 {
		static constexpr std::string_view ignores[] = {"CoreValidation-Shader-OutputNotConsumed"};
		std::string_view const msg = pCallbackData && pCallbackData->pMessage ? pCallbackData->pMessage : "UNKNOWN";
		if (messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			for (auto const ignore : ignores) {
				if (msg.find(ignore) != std::string_view::npos) { return false; }
			}
		}
		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
			trace::log({std::string(msg), "vf::Validation", trace::Type::eError});
			return true;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: VF_TRACEW("vf::Validation", "{}", msg); break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: VF_TRACEI("vf::Validation", "{}", msg); break;
		default: break;
		}
		return false;
	};

	auto dumci = vk::DebugUtilsMessengerCreateInfoEXT{};
	using vksev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
	dumci.messageSeverity = vksev::eError | vksev::eWarning | vksev::eInfo;
	using vktype = vk::DebugUtilsMessageTypeFlagBitsEXT;
	dumci.messageType = vktype::eGeneral | vktype::ePerformance | vktype::eValidation;
	dumci.pfnUserCallback = validationCallback;
	return instance.createDebugUtilsMessengerEXTUnique(dumci, nullptr);
}

Gpu make_gpu(vk::PhysicalDevice const& device, vk::SurfaceKHR surface) {
	auto ret = Gpu{};
	auto const properties = device.getProperties();
	ret.name = properties.deviceName.data();
	switch (properties.deviceType) {
	case vk::PhysicalDeviceType::eCpu: ret.type = Gpu::Type::eCpu; break;
	case vk::PhysicalDeviceType::eDiscreteGpu: ret.type = Gpu::Type::eDiscrete; break;
	case vk::PhysicalDeviceType::eIntegratedGpu: ret.type = Gpu::Type::eIntegrated; break;
	case vk::PhysicalDeviceType::eVirtualGpu: ret.type = Gpu::Type::eVirtual; break;
	case vk::PhysicalDeviceType::eOther: ret.type = Gpu::Type::eOther; break;
	default: break;
	}
	ret.max_line_width = properties.limits.lineWidthRange[1];
	auto const features = device.getFeatures();
	if (features.fillModeNonSolid) { ret.features.set(Gpu::Feature::eWireframe); }
	if (features.samplerAnisotropy) { ret.features.set(Gpu::Feature::eAnisotropicFiltering); }
	if (features.sampleRateShading) { ret.features.set(Gpu::Feature::eMsaa); }
	if (features.wideLines) { ret.features.set(Gpu::Feature::eWideLines); }
	for (auto const mode : device.getSurfacePresentModesKHR(surface)) { ret.present_modes.set(to_vsync(mode)); }
	return ret;
}

constexpr std::array required_extensions_v = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#if defined(VULKIFY_VK_PORTABILITY)
	"VK_KHR_portability_subset",
#endif
};

std::vector<PhysicalDevice> valid_devices(vk::Instance instance, vk::SurfaceKHR surface) {
	auto const has_all_extensions = [](vk::PhysicalDevice const& device) {
		auto remain = std::unordered_set<std::string_view>{required_extensions_v.begin(), required_extensions_v.end()};
		for (auto const& extension : device.enumerateDeviceExtensionProperties()) {
			auto const& name = extension.extensionName;
			if (remain.contains(name)) { remain.erase(name); }
		}
		return remain.empty();
	};
	auto const has_all_features = [](vk::PhysicalDevice const& device) {
		auto const& features = device.getFeatures();
		return features.fillModeNonSolid && features.samplerAnisotropy && features.sampleRateShading;
	};
	auto const get_queue_family = [surface](vk::PhysicalDevice const& device, std::uint32_t& out_family) {
		static constexpr auto queue_flags_v = vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eTransfer;
		auto const properties = device.getQueueFamilyProperties();
		for (auto const& [props, family] : ktl::enumerate<std::uint32_t>(properties)) {
			if (!device.getSurfaceSupportKHR(family, surface)) { continue; }
			if (!(props.queueFlags & queue_flags_v)) { continue; }
			out_family = family;
			return true;
		}
		return false;
	};
	auto available = instance.enumeratePhysicalDevices();
	auto ret = std::vector<PhysicalDevice>{};
	for (auto const& device : available) {
		if (!has_all_extensions(device) || !has_all_features(device)) { continue; }
		auto pd = PhysicalDevice{make_gpu(device, surface), device};
		if (!get_queue_family(device, pd.queueFamily)) { continue; }
		if (device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) { pd.score -= 100; }
		if (device.getFeatures().wideLines) { pd.score -= 10; }
		ret.push_back(std::move(pd));
	}
	std::sort(ret.begin(), ret.end());
	return ret;
}

vk::UniqueDevice make_device(std::span<char const*> layers, PhysicalDevice const& device) {
	static constexpr float priority_v = 1.0f;
	auto qci = vk::DeviceQueueCreateInfo({}, device.queueFamily, 1, &priority_v);
	auto dci = vk::DeviceCreateInfo{};
	auto enabled = vk::PhysicalDeviceFeatures{};
	auto available = device.device.getFeatures();
	enabled.fillModeNonSolid = available.fillModeNonSolid;
	enabled.wideLines = available.wideLines;
	enabled.samplerAnisotropy = available.samplerAnisotropy;
	enabled.sampleRateShading = available.sampleRateShading;
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
	dci.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
	dci.ppEnabledLayerNames = layers.data();
	dci.enabledExtensionCount = static_cast<std::uint32_t>(required_extensions_v.size());
	dci.ppEnabledExtensionNames = required_extensions_v.data();
	dci.pEnabledFeatures = &enabled;
	return device.device.createDeviceUnique(dci);
}
} // namespace

std::vector<Gpu> VulkanInstance::available_devices() const {
	auto ret = std::vector<Gpu>{};
	if (instance && surface) {
		auto devices = valid_devices(*instance, *surface);
		ret.reserve(devices.size());
		for (auto const& device : devices) { ret.push_back(make_gpu(device.device, *surface)); }
	}
	return ret;
}

Result<VulkanInstance::Builder> VulkanInstance::Builder::make(Info info, bool validation) {
	if (!info.make_surface) { return Error::eInvalidArgument; }
	auto ret = Builder{};
	auto dl = vk::DynamicLoader{};
	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	ret.instance.instance = make_instance(info.instance_extensions, validation);
	ret.validation = validation;
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*ret.instance.instance);
	if (validation) { ret.instance.messenger = make_debug_messenger(*ret.instance.instance); }
	auto surface = info.make_surface(*ret.instance.instance);
	if (!surface) { return Error::eVulkanInitFailure; }
	ret.instance.surface = vk::UniqueSurfaceKHR(surface, *ret.instance.instance);
	ret.devices = valid_devices(*ret.instance.instance, *ret.instance.surface);
	if (ret.devices.empty()) { return Error::eNoVulkanSupport; }
	return Result<Builder>(std::move(ret));
}

Result<VulkanInstance> VulkanInstance::Builder::operator()(PhysicalDevice&& selected) {
	instance.gpu.gpu = std::move(selected.gpu);
	instance.gpu.device = selected.device;
	instance.gpu.formats = selected.device.getSurfaceFormatsKHR(*instance.surface);
	instance.gpu.properties = selected.device.getProperties();
	auto layers = ktl::fixed_vector<char const*, 2>{};
	if (validation) { layers.push_back(validation_layer_v.data()); }
	instance.device = make_device(layers, selected);
	if (!instance.device) { return Error::eVulkanInitFailure; }

	VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance.device);
	instance.queue = Queue{instance.device->getQueue(selected.queueFamily, 0), selected.queueFamily};
	instance.util = ktl::make_unique<Util>();
	instance.util->device_limits = instance.gpu.device.getProperties().limits;
	return std::move(instance);
}
/// /Instance

/// CommandPool
FencePool::FencePool(VulkanDevice device, std::size_t count) : m_device(device) {
	if (!m_device) { return; }
	m_idle.reserve(count);
	for (std::size_t i = 0; i < count; ++i) { m_idle.push_back(make_fence()); }
}

vk::Fence FencePool::next() {
	if (!m_device) { return {}; }
	std::erase_if(m_busy, [this](vk::UniqueFence& f) {
		if (!m_device.busy(*f)) {
			m_idle.push_back(std::move(f));
			return true;
		}
		return false;
	});
	if (m_idle.empty()) { m_idle.push_back(make_fence()); }
	auto ret = std::move(m_idle.back());
	m_idle.pop_back();
	m_device.reset(*ret, {});
	m_busy.push_back(std::move(ret));
	return *m_busy.back();
}

vk::UniqueFence FencePool::make_fence() const { return m_device.device.createFenceUnique({vk::FenceCreateFlagBits::eSignaled}); }

CommandPool::CommandPool(VulkanDevice device, std::size_t batch) : m_device(device), m_fencePool(device, 0U), m_batch(static_cast<std::uint32_t>(batch)) {
	if (!m_device) { return; }
	m_pool = m_device.device.createCommandPoolUnique({pool_flags_v, m_device.queue.family});
	assert(m_pool);
}

vk::CommandBuffer CommandPool::acquire() {
	if (!m_device || !m_pool) { return {}; }
	Cmd cmd;
	for (auto& cb : m_cbs) {
		if (!m_device.busy(cb.fence)) {
			std::swap(cb, m_cbs.back());
			cmd = std::move(m_cbs.back());
			break;
		}
	}
	if (!cmd.cb) {
		auto const info = vk::CommandBufferAllocateInfo(*m_pool, vk::CommandBufferLevel::ePrimary, m_batch);
		auto cbs = m_device.device.allocateCommandBuffers(info);
		m_cbs.reserve(m_cbs.size() + cbs.size());
		for (auto const cb : cbs) { m_cbs.push_back(Cmd{{}, cb, {}}); }
		cmd = std::move(m_cbs.back());
	}
	m_cbs.pop_back();
	auto ret = cmd.cb;
	ret.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	return ret;
}

vk::Result CommandPool::release(vk::CommandBuffer&& cb, bool block, DeferQueue&& defer) {
	auto ret = vk::Result::eErrorDeviceLost;
	if (!m_device) { return ret; }
	assert(!std::any_of(m_cbs.begin(), m_cbs.end(), [c = cb](Cmd const& cmd) { return cmd.cb == c; }));
	cb.end();
	Cmd cmd{std::move(defer), cb, m_fencePool.next()};
	vk::SubmitInfo const si(0U, nullptr, {}, 1U, &cb);
	{
		auto lock = std::scoped_lock(*m_device.queue_mutex);
		ret = m_device.queue.queue.submit(1, &si, cmd.fence);
	}
	if (ret == vk::Result::eSuccess) {
		if (block) {
			m_device.wait(cmd.fence);
			cmd.defer.entries.clear();
		}
	} else {
		m_device.reset(cmd.fence, {});
	}
	m_cbs.push_back(std::move(cmd));
	return ret;
}

void CommandPool::clear() {
	m_device.device.waitIdle();
	m_cbs.clear();
}

/// /CommandPool

/// GfxDevice
namespace {
vk::SampleCountFlagBits get_samples(vk::SampleCountFlags supported, int desired) {
	if (desired >= 16 && (supported & vk::SampleCountFlagBits::e16)) { return vk::SampleCountFlagBits::e16; }
	if (desired >= 8 && (supported & vk::SampleCountFlagBits::e8)) { return vk::SampleCountFlagBits::e8; }
	if (desired >= 4 && (supported & vk::SampleCountFlagBits::e4)) { return vk::SampleCountFlagBits::e4; }
	if (desired >= 2 && (supported & vk::SampleCountFlagBits::e2)) { return vk::SampleCountFlagBits::e2; }
	return vk::SampleCountFlagBits::e1;
}

[[maybe_unused]] constexpr auto name_v = "vf::(internal)";

std::atomic<std::uint64_t> g_next_id{};
} // namespace

BlitCaps BlitCaps::make(vk::PhysicalDevice device, vk::Format format) {
	BlitCaps ret;
	auto const fp = device.getFormatProperties(format);
	using vFFFB = vk::FormatFeatureFlagBits;
	if (fp.linearTilingFeatures & vFFFB::eBlitSrc) { ret.linear.set(BlitFlag::eSrc); }
	if (fp.linearTilingFeatures & vFFFB::eBlitDst) { ret.linear.set(BlitFlag::eDst); }
	if (fp.linearTilingFeatures & vFFFB::eSampledImageFilterLinear) { ret.linear.set(BlitFlag::eLinearFilter); }
	if (fp.optimalTilingFeatures & vFFFB::eBlitSrc) { ret.optimal.set(BlitFlag::eSrc); }
	if (fp.optimalTilingFeatures & vFFFB::eBlitDst) { ret.optimal.set(BlitFlag::eDst); }
	if (fp.optimalTilingFeatures & vFFFB::eSampledImageFilterLinear) { ret.optimal.set(BlitFlag::eLinearFilter); }
	return ret;
}

vk::ImageLayout ImageBarrier::operator()(vk::CommandBuffer cb, vk::Image image, TPair<vk::ImageLayout> layouts) const {
	if (layouts.second == layouts.first || layouts.second == vk::ImageLayout::eUndefined) { return layouts.first; }
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = layouts.first;
	barrier.newLayout = layouts.second;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspects;
	barrier.subresourceRange.baseMipLevel = layer_mip.mip.first;
	barrier.subresourceRange.levelCount = layer_mip.mip.count;
	barrier.subresourceRange.baseArrayLayer = layer_mip.layer.first;
	barrier.subresourceRange.layerCount = layer_mip.layer.count;
	barrier.srcAccessMask = access.first;
	barrier.dstAccessMask = access.second;
	cb.pipelineBarrier(stages.first, stages.second, {}, {}, {}, barrier);
	return layouts.second;
}

vk::ImageLayout ImageBarrier::operator()(vk::CommandBuffer cb, vk::Image image, vk::ImageLayout target) const {
	return operator()(cb, image, {vk::ImageLayout::eUndefined, target});
}

bool VmaBuffer::write(void const* data, std::size_t size) {
	if (!map || this->size < size) { return false; }
	std::memcpy(map, data, size);
	return true;
}

void VmaBuffer::Deleter::operator()(VmaBuffer const& buffer) const {
	if (!buffer.handle) { return; }
	if (buffer.map) { vmaUnmapMemory(buffer.allocator, buffer.handle); }
	vmaDestroyBuffer(buffer.allocator, buffer.resource, buffer.handle);
}

void VmaImage::transition(vk::CommandBuffer cb, vk::ImageLayout to, ImageBarrier const& barrier) { layout = barrier(cb, resource, {layout, to}); }

void VmaImage::Deleter::operator()(const VmaImage& image) const {
	if (!image.handle) { return; }
	vmaDestroyImage(image.allocator, image.resource, image.handle);
}

UniqueGfxDevice UniqueGfxDevice::make(VulkanInstance const& instance, FT_Library ft, int samples) {
	auto allocatorInfo = VmaAllocatorCreateInfo{};
	auto dl = VULKAN_HPP_DEFAULT_DISPATCHER;
	allocatorInfo.instance = static_cast<VkInstance>(*instance.instance);
	allocatorInfo.device = static_cast<VkDevice>(*instance.device);
	allocatorInfo.physicalDevice = static_cast<VkPhysicalDevice>(instance.gpu.device);
	auto vkFunc = VmaVulkanFunctions{};
	vkFunc.vkGetInstanceProcAddr = dl.vkGetInstanceProcAddr;
	vkFunc.vkGetDeviceProcAddr = dl.vkGetDeviceProcAddr;
	allocatorInfo.pVulkanFunctions = &vkFunc;
	auto ret = GfxDevice{VulkanDevice::make(instance)};
	if (vmaCreateAllocator(&allocatorInfo, &ret.allocator) != VK_SUCCESS) {
		VF_TRACE(name_v, trace::Type::eError, "Failed to create GfxDevice!");
		return {};
	}
	auto factory = ktl::make_unique<CommandFactory>(CommandPoolFactory{ret.device});
	ret.command_factory = factory.get();
	ret.device_limits = ret.device.limits;
	assert(ret.device.limits);
	ret.colour_samples = get_samples(ret.device.limits->framebufferColorSampleCounts, samples);
	ret.ftlib = ft;
	ret.defer = &instance.util->defer;
	return {std::move(factory), ret};
}

void GfxDevice::Deleter::operator()(GfxDevice const& device) const {
	if (!device.device) { return; }
	device.command_factory->clear();
	device.defer->entries.clear();
	auto lock = std::scoped_lock(device.allocations->mutex);
	device.allocations->clear(lock);
	vmaDestroyAllocator(device.allocator);
}

UniqueImage GfxDevice::make_image(vk::ImageCreateInfo info, bool host, bool linear) const {
	if (!allocator || !command_factory) { return {}; }
	if (info.extent.width > device_limits->maxImageDimension2D || info.extent.height > device_limits->maxImageDimension2D) {
		VF_TRACEW(name_v, "Invalid image extent: [{}, {}]", info.extent.width, info.extent.height);
		return {};
	}
	info.tiling = linear ? vk::ImageTiling::eLinear : vk::ImageTiling::eOptimal;
	if (info.imageType == vk::ImageType()) { info.imageType = vk::ImageType::e2D; }
	if (info.mipLevels == 0U) { info.mipLevels = 1U; }
	if (info.arrayLayers == 0U) { info.arrayLayers = 1U; }
	info.sharingMode = vk::SharingMode::eExclusive;
	info.queueFamilyIndexCount = 1U;
	info.pQueueFamilyIndices = &device.queue.family;

	auto vaci = VmaAllocationCreateInfo{};
	vaci.usage = host ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	auto const& imageInfo = static_cast<VkImageCreateInfo const&>(info);
	auto ret = VkImage{};
	auto handle = VmaAllocation{};
	if (auto res = vmaCreateImage(allocator, &imageInfo, &vaci, &ret, &handle, nullptr); res != VK_SUCCESS) { return {}; }

	auto const caps = BlitCaps::make(device.gpu, info.format);
	auto const id = ++g_next_id;
	return VmaImage{{vk::Image(ret), allocator, handle, id}, info.initialLayout, info.extent, info.tiling, caps};
}

UniqueBuffer GfxDevice::make_buffer(vk::BufferCreateInfo info, bool host) const {
	if (!command_factory || !allocator) { return {}; }
	info.sharingMode = vk::SharingMode::eExclusive;
	info.queueFamilyIndexCount = 1U;
	info.pQueueFamilyIndices = &device.queue.family;

	auto vaci = VmaAllocationCreateInfo{};
	vaci.usage = host ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	if (host) { vaci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT; }
	auto const& vkBufferInfo = static_cast<VkBufferCreateInfo>(info);
	auto ret = VkBuffer{};
	auto handle = VmaAllocation{};
	if (vmaCreateBuffer(allocator, &vkBufferInfo, &vaci, &ret, &handle, nullptr) != VK_SUCCESS) { return {}; }
	void* map{};
	if (host) { vmaMapMemory(allocator, handle, &map); }

	auto const id = ++g_next_id;
	return VmaBuffer{{vk::Buffer(ret), allocator, handle, id}, info.size, map};
}

vk::SamplerCreateInfo GfxDevice::sampler_info(vk::SamplerAddressMode mode, vk::Filter filter) const {
	auto ret = vk::SamplerCreateInfo{};
	ret.minFilter = ret.magFilter = filter;
	ret.anisotropyEnable = device_limits->maxSamplerAnisotropy > 0.0f;
	ret.maxAnisotropy = device_limits->maxSamplerAnisotropy;
	ret.borderColor = vk::BorderColor::eIntOpaqueBlack;
	ret.mipmapMode = vk::SamplerMipmapMode::eNearest;
	ret.addressModeU = ret.addressModeV = ret.addressModeW = mode;
	return ret;
}
/// /GfxDevice

/// GfxBuffer/Image
vk::ImageCreateInfo& ImageCache::set_depth() {
	static constexpr auto flags = vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment;
	info.info = vk::ImageCreateInfo();
	info.info.usage = flags;
	info.aspect |= vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
	return info.info;
}

vk::ImageCreateInfo& ImageCache::set_colour() {
	static constexpr auto flags = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
	info.info = vk::ImageCreateInfo();
	info.info.usage = flags;
	info.aspect |= vk::ImageAspectFlagBits::eColor;
	return info.info;
}

vk::ImageCreateInfo& ImageCache::set_texture(bool const transfer_src) {
	static constexpr auto flags = vk::ImageUsageFlagBits::eSampled;
	info.info = vk::ImageCreateInfo();
	info.info.usage = flags;
	info.info.format = device->texture_format;
	info.info.usage |= vk::ImageUsageFlagBits::eTransferDst;
	if (transfer_src) { info.info.usage |= vk::ImageUsageFlagBits::eTransferSrc; }
	info.aspect |= vk::ImageAspectFlagBits::eColor;
	return info.info;
}

bool ImageCache::ready(Extent const extent, vk::Format const format) const noexcept {
	if (!image || info.info.format != format) { return false; }
	return current() == extent;
}

bool ImageCache::make(Extent const extent, vk::Format const format) {
	info.info.extent = vk::Extent3D(extent.x, extent.y, 1);
	info.info.format = format;
	device->defer->push(std::move(image));
	device->defer->push(std::move(view));
	image = device->make_image(info.info, info.prefer_host);
	if (!image) { return false; }
	view = device->device.make_image_view(image->resource, format, info.aspect);
	return *view;
}

ImageView ImageCache::refresh(Extent const extent, vk::Format format) {
	if (format == vk::Format()) { format = info.info.format; }
	if (!ready(extent, format)) { make(extent, format); }
	return peek();
}

BufferCache::BufferCache(GfxDevice const& device, vk::BufferUsageFlagBits usage) : device(device) {
	info.usage = usage;
	info.size = 1;
	for (std::size_t i = 0; i < device.buffering; ++i) { buffers.push(device.make_buffer(info, true)); }
}

VmaBuffer const& BufferCache::get(bool next) const {
	static auto const blank_v = VmaBuffer{};
	if (!device) { return blank_v; }
	auto& buffer = buffers.get();
	if (buffer->size < data.size()) {
		info.size = data.size();
		device.defer->push(std::move(buffer));
		buffer = device.make_buffer(info, true);
	}
	buffer->write(data.data(), data.size());
	if (next) { buffers.next(); }
	return buffer;
}

void GfxImage::replace(ImageCache&& cache) {
	device()->defer->push(std::move(image.cache));
	image.cache = std::move(cache);
}
/// /GfxBuffer/Image

/// GfxCommandBuffer
void ImageWriter::blit(vk::CommandBuffer cmd, vk::Image in, vk::Image out, TRect<std::int32_t> inr, TRect<std::int32_t> outr, vk::Filter filter) {
	auto isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	auto const src_offsets = std::array<vk::Offset3D, 2>{{{inr.offset.x, inr.offset.y}, {inr.extent.x, inr.extent.y, 1}}};
	auto const dst_offsets = std::array<vk::Offset3D, 2>{{{outr.offset.x, outr.offset.y}, {outr.extent.x, outr.extent.y, 1}}};
	auto ib = vk::ImageBlit(isrl, src_offsets, isrl, dst_offsets);
	cmd.blitImage(in, vk::ImageLayout::eTransferSrcOptimal, out, vk::ImageLayout::eTransferDstOptimal, ib, filter);
}

bool ImageWriter::can_blit(VmaImage const& src, VmaImage const& dst) { return src.blit_flags().test(BlitFlag::eSrc) && dst.blit_flags().test(BlitFlag::eDst); }

bool ImageWriter::write(VmaImage& out, std::span<std::byte const> data, URegion region, vk::ImageLayout il) {
	auto bci = vk::BufferCreateInfo({}, data.size(), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst);
	auto buffer = device->make_buffer(bci, true);
	if (!buffer || !buffer->write(data.data(), data.size())) { return false; }

	if (region.extent.x == 0 && region.extent.x == 0) {
		if (region.offset.x != 0 || region.offset.y != 0) { return false; }
		region.extent = {out.extent.width, out.extent.height};
	}
	if (region.extent.x == 0 || region.extent.y == 0) { return false; }

	auto const offset = glm::ivec2(region.offset);
	auto isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	auto icr = vk::ImageCopy(isrl, {}, isrl, {}, vk::Extent3D(region.extent.x, region.extent.y, 1));
	auto bic = vk::BufferImageCopy({}, {}, {}, isrl, vk::Offset3D(offset.x, offset.y, 0), icr.extent);
	out.transition(cb, vk::ImageLayout::eTransferDstOptimal);
	cb.copyBufferToImage(buffer->resource, out.resource, out.layout, bic);
	out.transition(cb, il);
	scratch.push_back(std::move(buffer));

	return true;
}

bool ImageWriter::blit(VmaImage& in, VmaImage& out, IRegion inr, IRegion outr, vk::Filter filter, TPair<vk::ImageLayout> il) const {
	if (!can_blit(in, out) || in.extent == out.extent) { return copy(in, out, inr, outr, il); }
	if (in.extent.width == 0 || in.extent.height == 0) { return false; }
	if (out.extent.width == 0 || out.extent.height == 0) { return false; }

	in.transition(cb, vk::ImageLayout::eTransferSrcOptimal);
	out.transition(cb, vk::ImageLayout::eTransferDstOptimal);
	blit(cb, in.resource, out.resource, inr, outr, filter);
	in.transition(cb, il.first);
	out.transition(cb, il.second);

	return true;
}

bool ImageWriter::copy(VmaImage& in, VmaImage& out, IRegion inr, IRegion outr, TPair<vk::ImageLayout> il) const {
	if (in.extent.width == 0 || in.extent.height == 0) { return false; }
	if (out.extent.width == 0 || out.extent.height == 0) { return false; }
	if (inr.extent != outr.extent) { return false; }

	auto isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	in.transition(cb, vk::ImageLayout::eTransferSrcOptimal);
	out.transition(cb, vk::ImageLayout::eTransferDstOptimal);
	auto const ino = vk::Offset3D(inr.offset.x, inr.offset.y, 0);
	auto const outo = vk::Offset3D(outr.offset.x, outr.offset.y, 0);
	auto const extent = glm::uvec2(inr.extent);
	auto ib = vk::ImageCopy(isrl, ino, isrl, outo, vk::Extent3D(extent.x, extent.y, 1));
	cb.copyImage(in.resource, in.layout, out.resource, out.layout, ib);
	in.transition(cb, il.first);
	out.transition(cb, il.second);

	return true;
}

void ImageWriter::clear(VmaImage& in, Rgba rgba) const {
	auto const c = rgba.normalize();
	auto const colour = std::array{c.x, c.y, c.z, c.w};
	in.transition(cb, vk::ImageLayout::eTransferDstOptimal);
	auto isrr = vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
	cb.clearColorImage(in.resource, vk::ImageLayout::eTransferDstOptimal, colour, isrr);
}
/// /GfxCommandBuffer
} // namespace refactor
} // namespace vf
