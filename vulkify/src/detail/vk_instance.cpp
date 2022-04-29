#include <VkBootstrap.h>
#include <detail/vk_instance.hpp>

namespace vf {
Result<VKInstance> VKInstance::make(MakeSurface const makeSurface, bool const validation) {
	if (!makeSurface) { return Error::eInvalidArgument; }
	auto dl = vk::DynamicLoader{};
	VULKAN_HPP_DEFAULT_DISPATCHER.init(dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr"));
	auto vib = vkb::InstanceBuilder{};
	if (validation) { vib.request_validation_layers(); }
	vib.set_app_name("vulkify").use_default_debug_messenger();
#if defined(VULKIFY_REQUIRE_VULKAN_1_1_0)
	vib.require_api_version(1, 1, 0);
#endif
	auto vi = vib.build();
	if (!vi) { return Error::eVulkanInitFailure; }
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vi->instance);
	auto ret = VKInstance{};
	ret.instance = vk::UniqueInstance(vi->instance, {nullptr});
	ret.messenger = vk::UniqueDebugUtilsMessengerEXT(vi->debug_messenger, {vi->instance});
	auto surface = makeSurface(vk::Instance(vi->instance));
	if (!surface) { return Error::eVulkanInitFailure; }
	ret.surface = vk::UniqueSurfaceKHR(surface, {vi->instance});
	auto vpds = vkb::PhysicalDeviceSelector(vi.value());
	auto features = vk::PhysicalDeviceFeatures{};
	features.fillModeNonSolid = true;
	features.samplerAnisotropy = true;
	vpds.set_required_features(static_cast<VkPhysicalDeviceFeatures>(features));
	auto vpd = vpds.require_present().prefer_gpu_device_type(vkb::PreferredDeviceType::discrete).set_surface(surface).select();
	if (!vpd) { return Error::eVulkanInitFailure; }
	ret.gpu.properties = vk::PhysicalDeviceProperties(vpd->properties);
	ret.gpu.device = vk::PhysicalDevice(vpd->physical_device);
	ret.gpu.formats = ret.gpu.device.getSurfaceFormatsKHR(*ret.surface);
	auto vdb = vkb::DeviceBuilder(vpd.value());
	auto vd = vdb.build();
	if (!vd) { return Error::eVulkanInitFailure; }
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vd->device);
	ret.device = vk::UniqueDevice(vd->device, {nullptr});
	auto queue = vd->get_queue(vkb::QueueType::graphics);
	auto qfam = vd->get_queue_index(vkb::QueueType::graphics);
	if (!queue || !qfam) { return Error::eVulkanInitFailure; }
	ret.queue = VKQueue{vk::Queue(queue.value()), qfam.value()};
	ret.util = ktl::make_unique<Util>();
	return ret;
}
} // namespace vf
