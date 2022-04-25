#include <detail/trace.hpp>
#include <detail/vram.hpp>

namespace vf {
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

void ImageBarrier::operator()(vk::CommandBuffer cb, vk::Image image) const {
	vk::ImageMemoryBarrier barrier;
	barrier.oldLayout = layouts.first;
	barrier.newLayout = layouts.second;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspects;
	barrier.subresourceRange.baseMipLevel = layerMip.mip.first;
	barrier.subresourceRange.levelCount = layerMip.mip.count;
	barrier.subresourceRange.baseArrayLayer = layerMip.layer.first;
	barrier.subresourceRange.layerCount = layerMip.layer.count;
	barrier.srcAccessMask = access.first;
	barrier.dstAccessMask = access.second;
	cb.pipelineBarrier(stages.first, stages.second, {}, {}, {}, barrier);
}

UniqueVram makeVram(vk::Instance instance, VKDevice device) {
	VmaAllocatorCreateInfo allocatorInfo = {};
	auto dl = VULKAN_HPP_DEFAULT_DISPATCHER;
	allocatorInfo.instance = static_cast<VkInstance>(instance);
	allocatorInfo.device = static_cast<VkDevice>(device.device);
	allocatorInfo.physicalDevice = static_cast<VkPhysicalDevice>(device.gpu);
	VmaVulkanFunctions vkFunc = {};
	vkFunc.vkGetPhysicalDeviceProperties = dl.vkGetPhysicalDeviceProperties;
	vkFunc.vkGetPhysicalDeviceMemoryProperties = dl.vkGetPhysicalDeviceMemoryProperties;
	vkFunc.vkAllocateMemory = dl.vkAllocateMemory;
	vkFunc.vkFreeMemory = dl.vkFreeMemory;
	vkFunc.vkMapMemory = dl.vkMapMemory;
	vkFunc.vkUnmapMemory = dl.vkUnmapMemory;
	vkFunc.vkFlushMappedMemoryRanges = dl.vkFlushMappedMemoryRanges;
	vkFunc.vkInvalidateMappedMemoryRanges = dl.vkInvalidateMappedMemoryRanges;
	vkFunc.vkBindBufferMemory = dl.vkBindBufferMemory;
	vkFunc.vkBindImageMemory = dl.vkBindImageMemory;
	vkFunc.vkGetBufferMemoryRequirements = dl.vkGetBufferMemoryRequirements;
	vkFunc.vkGetImageMemoryRequirements = dl.vkGetImageMemoryRequirements;
	vkFunc.vkCreateBuffer = dl.vkCreateBuffer;
	vkFunc.vkDestroyBuffer = dl.vkDestroyBuffer;
	vkFunc.vkCreateImage = dl.vkCreateImage;
	vkFunc.vkDestroyImage = dl.vkDestroyImage;
	vkFunc.vkCmdCopyBuffer = dl.vkCmdCopyBuffer;
	vkFunc.vkGetBufferMemoryRequirements2KHR = dl.vkGetBufferMemoryRequirements2KHR;
	vkFunc.vkGetImageMemoryRequirements2KHR = dl.vkGetImageMemoryRequirements2KHR;
	vkFunc.vkBindBufferMemory2KHR = dl.vkBindBufferMemory2KHR;
	vkFunc.vkBindImageMemory2KHR = dl.vkBindImageMemory2KHR;
	vkFunc.vkGetPhysicalDeviceMemoryProperties2KHR = dl.vkGetPhysicalDeviceMemoryProperties2KHR;
	allocatorInfo.pVulkanFunctions = &vkFunc;
	auto ret = Vram{};
	if (vmaCreateAllocator(&allocatorInfo, &ret.allocator) != VK_SUCCESS) {
		VF_TRACE("Failed to create Vram!");
		return {};
	}
	VF_TRACE("Vram constructed");
	ret.device = device;
	return ret;
}

void Vram::Deleter::operator()(Vram const& vram) const { vmaDestroyAllocator(vram.allocator); }

UniqueImage Vram::makeImage(vk::ImageCreateInfo info, VmaMemoryUsage const usage, bool linear) const {
	if (!allocator || !device.device) { return {}; }
	info.tiling = linear ? vk::ImageTiling::eLinear : vk::ImageTiling::eOptimal;
	if (info.imageType == vk::ImageType()) { info.imageType = vk::ImageType::e2D; }
	if (info.mipLevels == 0U) { info.mipLevels = 1U; }
	if (info.arrayLayers == 0U) { info.arrayLayers = 1U; }
	info.sharingMode = vk::SharingMode::eExclusive;
	info.queueFamilyIndexCount = 1U;
	info.pQueueFamilyIndices = &device.queue.family;

	auto vaci = VmaAllocationCreateInfo{};
	vaci.usage = usage;
	auto const& imageInfo = static_cast<VkImageCreateInfo const&>(info);
	auto ret = VkImage{};
	auto handle = VmaAllocation{};
	if (auto res = vmaCreateImage(allocator, &imageInfo, &vaci, &ret, &handle, nullptr); res != VK_SUCCESS) { return {}; }
	return VmaImage{vk::Image(ret), allocator, handle};
}

UniqueBuffer Vram::makeBuffer(vk::BufferCreateInfo info, VmaMemoryUsage const usage) const {
	info.sharingMode = vk::SharingMode::eExclusive;
	info.queueFamilyIndexCount = 1U;
	info.pQueueFamilyIndices = &device.queue.family;

	auto vaci = VmaAllocationCreateInfo{};
	vaci.usage = usage;
	auto const& vkBufferInfo = static_cast<VkBufferCreateInfo>(info);
	auto ret = VkBuffer{};
	auto handle = VmaAllocation{};
	if (vmaCreateBuffer(allocator, &vkBufferInfo, &vaci, &ret, &handle, nullptr) != VK_SUCCESS) { return {}; }
	return VmaBuffer{vk::Buffer(ret), allocator, handle};
}
} // namespace vf
