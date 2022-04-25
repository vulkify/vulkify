#include <detail/command_pool.hpp>
#include <detail/trace.hpp>
#include <detail/vram.hpp>
#include <ktl/fixed_vector.hpp>
#include <atomic>

namespace vf {
namespace {
constexpr bool hostVisible(VmaMemoryUsage usage) noexcept {
	return usage == VMA_MEMORY_USAGE_CPU_TO_GPU || usage == VMA_MEMORY_USAGE_GPU_TO_CPU || usage == VMA_MEMORY_USAGE_CPU_ONLY ||
		   usage == VMA_MEMORY_USAGE_CPU_COPY;
}

char const* getName(char const* name, std::string& fallback, std::atomic<int>& count) {
	if (!name) {
		fallback = ktl::str_format("unnamed_{}", count++);
		name = fallback.c_str();
	}
	return name;
}
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

UniqueVram UniqueVram::make(vk::Instance instance, VKDevice device) {
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
	auto vram = Vram{};
	if (vmaCreateAllocator(&allocatorInfo, &vram.allocator) != VK_SUCCESS) {
		VF_TRACE("Failed to create Vram!");
		return {};
	}
	VF_TRACE("Vram constructed");
	auto commandPool = ktl::make_unique<CommandPool>(device);
	vram.commandPool = commandPool.get();
	return {std::move(commandPool), vram};
}

void Vram::Deleter::operator()(Vram const& vram) const {
	vram.commandPool->clear();
	vmaDestroyAllocator(vram.allocator);
}

UniqueImage Vram::makeImage(vk::ImageCreateInfo info, VmaMemoryUsage const usage, char const* name, bool linear) const {
	if (!allocator || !commandPool) { return {}; }
	info.tiling = linear ? vk::ImageTiling::eLinear : vk::ImageTiling::eOptimal;
	if (info.imageType == vk::ImageType()) { info.imageType = vk::ImageType::e2D; }
	if (info.mipLevels == 0U) { info.mipLevels = 1U; }
	if (info.arrayLayers == 0U) { info.arrayLayers = 1U; }
	info.sharingMode = vk::SharingMode::eExclusive;
	info.queueFamilyIndexCount = 1U;
	info.pQueueFamilyIndices = &commandPool->m_device.queue.family;

	auto vaci = VmaAllocationCreateInfo{};
	vaci.usage = usage;
	auto const& imageInfo = static_cast<VkImageCreateInfo const&>(info);
	auto ret = VkImage{};
	auto handle = VmaAllocation{};
	if (auto res = vmaCreateImage(allocator, &imageInfo, &vaci, &ret, &handle, nullptr); res != VK_SUCCESS) { return {}; }
	void* map{};
	if (hostVisible(usage)) { vmaMapMemory(allocator, handle, &map); }

	if (commandPool->m_device.hasDebugMessenger) {
		static auto count = std::atomic<int>{};
		auto nameFallback = std::string{};
		name = getName(name, nameFallback, count);
		commandPool->m_device.setDebugName(vk::ObjectType::eImage, ret, name);
	}
	return VmaImage{vk::Image(ret), allocator, handle, map};
}

UniqueBuffer Vram::makeBuffer(vk::BufferCreateInfo info, VmaMemoryUsage const usage, char const* name) const {
	if (!commandPool || !allocator) { return {}; }
	info.sharingMode = vk::SharingMode::eExclusive;
	info.queueFamilyIndexCount = 1U;
	info.pQueueFamilyIndices = &commandPool->m_device.queue.family;

	auto vaci = VmaAllocationCreateInfo{};
	vaci.usage = usage;
	auto const& vkBufferInfo = static_cast<VkBufferCreateInfo>(info);
	auto ret = VkBuffer{};
	auto handle = VmaAllocation{};
	if (vmaCreateBuffer(allocator, &vkBufferInfo, &vaci, &ret, &handle, nullptr) != VK_SUCCESS) { return {}; }
	void* map{};
	if (hostVisible(usage)) { vmaMapMemory(allocator, handle, &map); }

	if (commandPool->m_device.hasDebugMessenger) {
		static auto id = std::atomic<int>{};
		auto nameFallback = std::string{};
		name = getName(name, nameFallback, id);
		commandPool->m_device.setDebugName(vk::ObjectType::eBuffer, ret, name);
	}

	return VmaBuffer{vk::Buffer(ret), allocator, handle, map};
}

VIBuffer Vram::makeVIBuffer(Geometry const& geometry, VIBuffer::Type type, char const* name) const {
	if (geometry.vertices.empty()) { return {}; }
	auto ret = VIBuffer{};
	ret.type = type;
	bool const gpuOnly = type == VIBuffer::Type::eStatic;
	auto const usage = gpuOnly ? VMA_MEMORY_USAGE_GPU_ONLY : VMA_MEMORY_USAGE_CPU_TO_GPU;

	auto bci = vk::BufferCreateInfo{};
	bci.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	if (gpuOnly) { bci.usage |= vk::BufferUsageFlagBits::eTransferDst; }
	static auto count = std::atomic<int>{};
	auto nameFallback = std::string{};
	name = getName(name, nameFallback, count);
	bci.size = geometry.vertices.size() * sizeof(Vertex);
	auto str = ktl::str_format("{}_vbo", name);
	ret.vertex = makeBuffer(bci, usage, str.c_str());
	if (!geometry.indices.empty()) {
		bci.usage = vk::BufferUsageFlagBits::eIndexBuffer;
		bci.size = geometry.indices.size();
		str = ktl::str_format("{}_ibo", name);
		ret.index = makeBuffer(bci, usage, str.c_str());
	}
	auto cmd = commandPool->acquire();
	assert(cmd);

	auto scratch = ktl::fixed_vector<UniqueBuffer, 4>{};
	auto write = [&]<typename T>(UniqueBuffer& out, std::vector<T> const& data) {
		if (!out) { return; }
		assert(out->map);
		std::memcpy(out->map, data.data(), data.size() * sizeof(T));
	};
	auto copy = [&]<typename T>(UniqueBuffer& out, std::vector<T> const& data) {
		if (!out) { return; }
		bci.usage = vk::BufferUsageFlagBits::eTransferSrc;
		bci.size = data.size() * sizeof(T);
		str = ktl::str_format("{}_staging", name);
		auto stage = makeBuffer(bci, VMA_MEMORY_USAGE_CPU_TO_GPU, str.c_str());
		write(stage, data);
		cmd.copyBuffer(stage->resource, out->resource, vk::BufferCopy({}, {}, bci.size));
		scratch.push_back(std::move(stage));
	};
	if (gpuOnly) {
		copy(ret.vertex, geometry.vertices);
		copy(ret.index, geometry.indices);
	} else {
		write(ret.vertex, geometry.vertices);
		write(ret.index, geometry.indices);
	}

	commandPool->release(std::move(cmd), true);
	return ret;
}
} // namespace vf
