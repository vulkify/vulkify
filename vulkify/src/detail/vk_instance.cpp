#include <detail/trace.hpp>
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

vk::UniqueInstance makeInstance(std::vector<char const*> extensions, bool& out_validation) {
	if (out_validation) {
		auto const availLayers = vk::enumerateInstanceLayerProperties();
		auto layerSearch = [](vk::LayerProperties const& lp) { return lp.layerName == validation_layer_v; };
		if (std::find_if(availLayers.begin(), availLayers.end(), layerSearch) != availLayers.end()) {
			VF_TRACE("[vk::(internal)] Requesting Vulkan validation layers");
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		} else {
			VF_TRACE("[vk::(internal)] VK_LAYER_KHRONOS_validation not found");
			out_validation = false;
		}
	}

	auto const version = VK_MAKE_VERSION(version_v.major, version_v.minor, version_v.patch);
#if defined(VULKIFY_REQUIRE_VULKAN_1_1_0)
	static constexpr auto apiVersion = VK_API_VERSION_1_1;
#else
	static constexpr auto apiVersion = VK_API_VERSION_1_0;
#endif
	auto const ai = vk::ApplicationInfo("vulkify", version, "vulkify", version, apiVersion);
	auto ici = vk::InstanceCreateInfo{};
	// ici.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
	ici.pApplicationInfo = &ai;
	ici.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
	ici.ppEnabledExtensionNames = extensions.data();
	if (out_validation) {
		static char const* layers[] = {validation_layer_v.data()};
		ici.enabledLayerCount = 1;
		ici.ppEnabledLayerNames = layers;
	}
	return vk::createInstanceUnique(ici);
}

vk::UniqueDebugUtilsMessengerEXT makeDebugMessenger(vk::Instance instance) {
	auto validationCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT,
								 VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData, void*) -> vk::Bool32 {
		std::string_view const msg = pCallbackData && pCallbackData->pMessage ? pCallbackData->pMessage : "UNKNOWN";
		switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
			std::fprintf(stderr, "[vf::Validation] [E] %s", msg.data());
			return true;
		}
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: VF_TRACEF("[vf::Validation] [W] {}", msg); break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: VF_TRACEF("[vf::Validation] [I] {}", msg); break;
		default: break;
		}
		return false;
	};

	vk::DebugUtilsMessengerCreateInfoEXT createInfo;
	using vksev = vk::DebugUtilsMessageSeverityFlagBitsEXT;
	createInfo.messageSeverity = vksev::eError | vksev::eWarning | vksev::eInfo;
	using vktype = vk::DebugUtilsMessageTypeFlagBitsEXT;
	createInfo.messageType = vktype::eGeneral | vktype::ePerformance | vktype::eValidation;
	createInfo.pfnUserCallback = validationCallback;
	return instance.createDebugUtilsMessengerEXTUnique(createInfo, nullptr);
}

struct PhysicalDevice {
	vk::PhysicalDevice device{};
	std::uint32_t queueFamily{};
	int score{};

	auto operator<=>(PhysicalDevice const& rhs) const { return score <=> rhs.score; }
};

constexpr std::array required_extensions_v = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_MAINTENANCE1_EXTENSION_NAME,
#if defined(VULKIFY_VK_PORTABILITY)
	"VK_KHR_portability_subset",
#endif
};

constexpr auto requiredFeatures() {
	auto ret = vk::PhysicalDeviceFeatures{};
	ret.fillModeNonSolid = ret.samplerAnisotropy = ret.sampleRateShading = true;
	return ret;
}

std::vector<PhysicalDevice> validDevices(vk::Instance instance, vk::SurfaceKHR surface) {
	auto const hasAllExtensions = [](vk::PhysicalDevice const& device) {
		auto remain = std::unordered_set<std::string_view>{required_extensions_v.begin(), required_extensions_v.end()};
		for (auto const& extension : device.enumerateDeviceExtensionProperties()) {
			auto const& name = extension.extensionName;
			if (remain.contains(name)) { remain.erase(name); }
		}
		return remain.empty();
	};
	auto const hasAllFeatures = [](vk::PhysicalDevice const& device) {
		auto const& features = device.getFeatures();
		return features.fillModeNonSolid && features.samplerAnisotropy && features.sampleRateShading;
	};
	auto const getQueueFamily = [surface](vk::PhysicalDevice const& device, std::uint32_t& out_family) {
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
		if (!hasAllExtensions(device) || !hasAllFeatures(device)) { continue; }
		auto pd = PhysicalDevice{device};
		if (!getQueueFamily(device, pd.queueFamily)) { continue; }
		if (device.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) { pd.score -= 100; }
		if (device.getFeatures().wideLines) { pd.score -= 10; }
		ret.push_back(std::move(pd));
	}
	std::sort(ret.begin(), ret.end());
	return ret;
}

vk::UniqueDevice makeDevice(std::span<char const*> layers, PhysicalDevice const& device) {
	static constexpr float priority_v = 1.0f;
	auto qci = vk::DeviceQueueCreateInfo({}, device.queueFamily, 1, &priority_v);
	auto dci = vk::DeviceCreateInfo{};
	auto features = requiredFeatures();
	features.wideLines = device.device.getFeatures().wideLines;
	dci.queueCreateInfoCount = 1;
	dci.pQueueCreateInfos = &qci;
	dci.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
	dci.ppEnabledLayerNames = layers.data();
	dci.enabledExtensionCount = static_cast<std::uint32_t>(required_extensions_v.size());
	dci.ppEnabledExtensionNames = required_extensions_v.data();
	dci.pEnabledFeatures = &features;
	return device.device.createDeviceUnique(dci);
}
} // namespace

Result<VKInstance> VKInstance::make(Info info, bool validation) {
	if (!info.makeSurface) { return Error::eInvalidArgument; }
	auto ret = VKInstance{};
	auto dl = vk::DynamicLoader{};
	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	ret.instance = makeInstance(info.instanceExtensions, validation);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(*ret.instance);
	vk::UniqueDebugUtilsMessengerEXT messenger{};
	if (validation) { ret.messenger = makeDebugMessenger(*ret.instance); }
	auto surface = info.makeSurface(*ret.instance);
	if (!surface) { return Error::eVulkanInitFailure; }
	ret.surface = vk::UniqueSurfaceKHR(surface, *ret.instance);
	auto devices = validDevices(*ret.instance, *ret.surface);
	if (devices.empty()) { return Error::eNoVulkanSupport; }

	// TODO: select
	auto const& selected = devices.front();
	ret.gpu.device = selected.device;
	ret.gpu.formats = selected.device.getSurfaceFormatsKHR(surface);
	ret.gpu.properties = selected.device.getProperties();
	auto layers = ktl::fixed_vector<char const*, 2>{};
	if (validation) { layers.push_back(validation_layer_v.data()); }
	ret.device = makeDevice(layers, selected);
	if (!ret.device) { return Error::eVulkanInitFailure; }

	VULKAN_HPP_DEFAULT_DISPATCHER.init(*ret.device);
	ret.queue = VKQueue{ret.device->getQueue(selected.queueFamily, 0), selected.queueFamily};
	ret.util = ktl::make_unique<Util>();
	return ret;
}
} // namespace vf
