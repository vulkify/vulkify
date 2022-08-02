#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <detail/vk_device.hpp>
#include <detail/vk_instance.hpp>
#include <ktl/enumerate.hpp>
#include <ktl/fixed_vector.hpp>
#include <vulkify/vulkify_version.hpp>
#include <algorithm>
#include <span>
#include <unordered_set>

namespace vf {
using namespace std::string_view_literals;

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

std::vector<Gpu> VKInstance::available_devices() const {
	auto ret = std::vector<Gpu>{};
	if (instance && surface) {
		auto devices = valid_devices(*instance, *surface);
		ret.reserve(devices.size());
		for (auto const& device : devices) { ret.push_back(make_gpu(device.device, *surface)); }
	}
	return ret;
}

Result<VKInstance::Builder> VKInstance::Builder::make(Info info, bool validation) {
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

Result<VKInstance> VKInstance::Builder::operator()(PhysicalDevice&& selected) {
	instance.gpu.gpu = std::move(selected.gpu);
	instance.gpu.device = selected.device;
	instance.gpu.formats = selected.device.getSurfaceFormatsKHR(*instance.surface);
	instance.gpu.properties = selected.device.getProperties();
	auto layers = ktl::fixed_vector<char const*, 2>{};
	if (validation) { layers.push_back(validation_layer_v.data()); }
	instance.device = make_device(layers, selected);
	if (!instance.device) { return Error::eVulkanInitFailure; }

	VULKAN_HPP_DEFAULT_DISPATCHER.init(*instance.device);
	instance.queue = VKQueue{instance.device->getQueue(selected.queueFamily, 0), selected.queueFamily};
	instance.util = ktl::make_unique<Util>();
	return std::move(instance);
}

void VKDevice::wait(vk::Fence fence, std::uint64_t wait) const {
	if (fence) {
		auto res = device.waitForFences(1, &fence, true, static_cast<std::uint64_t>(wait));
		if (res != vk::Result::eSuccess) { VF_TRACE("vf::(internal)", trace::Type::eError, "Fence wait failure!"); }
	}
}

void VKDevice::reset(vk::Fence fence, std::uint64_t wait) const {
	if (wait > 0 && busy(fence)) { this->wait(fence, wait); }
	auto res = device.resetFences(1, &fence);
	if (res != vk::Result::eSuccess) { VF_TRACE("vf::(internal)", trace::Type::eError, "Fence reset failure!"); }
}
} // namespace vf
