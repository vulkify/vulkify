#include <detail/command_pool.hpp>
#include <detail/trace.hpp>
#include <detail/vram.hpp>
#include <ktl/fixed_vector.hpp>
#include <ktl/kformat.hpp>
#include <atomic>

namespace vf {
namespace {
char const* getName(char const* name, std::string& fallback, std::atomic<int>& count) {
	if (!name) {
		fallback = ktl::kformat("unnamed_{}", count++);
		name = fallback.c_str();
	}
	return name;
}

vk::SampleCountFlagBits getSamples(vk::SampleCountFlags supported, int desired) {
	if (desired >= 16 && (supported & vk::SampleCountFlagBits::e16)) { return vk::SampleCountFlagBits::e16; }
	if (desired >= 8 && (supported & vk::SampleCountFlagBits::e8)) { return vk::SampleCountFlagBits::e8; }
	if (desired >= 4 && (supported & vk::SampleCountFlagBits::e4)) { return vk::SampleCountFlagBits::e4; }
	if (desired >= 2 && (supported & vk::SampleCountFlagBits::e2)) { return vk::SampleCountFlagBits::e2; }
	return vk::SampleCountFlagBits::e1;
}

[[maybe_unused]] constexpr auto name_v = "vf::(internal)";
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
	barrier.subresourceRange.baseMipLevel = layerMip.mip.first;
	barrier.subresourceRange.levelCount = layerMip.mip.count;
	barrier.subresourceRange.baseArrayLayer = layerMip.layer.first;
	barrier.subresourceRange.layerCount = layerMip.layer.count;
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
	if (buffer.map) { vmaUnmapMemory(buffer.allocator, buffer.handle); }
	vmaDestroyBuffer(buffer.allocator, buffer.resource, buffer.handle);
}

void VmaImage::transition(vk::CommandBuffer cb, vk::ImageLayout to, ImageBarrier const& barrier) { layout = barrier(cb, resource, {layout, to}); }

void VmaImage::Deleter::operator()(const VmaImage& image) const { vmaDestroyImage(image.allocator, image.resource, image.handle); }

UniqueVram UniqueVram::make(vk::Instance instance, VKDevice device, int samples) {
	auto allocatorInfo = VmaAllocatorCreateInfo{};
	auto dl = VULKAN_HPP_DEFAULT_DISPATCHER;
	allocatorInfo.instance = static_cast<VkInstance>(instance);
	allocatorInfo.device = static_cast<VkDevice>(device.device);
	allocatorInfo.physicalDevice = static_cast<VkPhysicalDevice>(device.gpu);
	auto vkFunc = VmaVulkanFunctions{};
	vkFunc.vkGetInstanceProcAddr = dl.vkGetInstanceProcAddr;
	vkFunc.vkGetDeviceProcAddr = dl.vkGetDeviceProcAddr;
	allocatorInfo.pVulkanFunctions = &vkFunc;
	auto const limits = device.gpu.getProperties().limits;
	auto vram = Vram{device};
	if (vmaCreateAllocator(&allocatorInfo, &vram.allocator) != VK_SUCCESS) {
		VF_TRACE(name_v, trace::Type::eError, "Failed to create Vram!");
		return {};
	}
	auto factory = ktl::make_unique<CommandFactory>();
	factory->commandPools.factory().device = device;
	vram.commandFactory = factory.get();
	vram.deviceLimits = limits;
	vram.colourSamples = getSamples(limits.framebufferColorSampleCounts, samples);
	return {std::move(factory), vram};
}

void Vram::Deleter::operator()(Vram const& vram) const {
	vram.device.defer.queue->entries.clear();
	vram.commandFactory->commandPools.clear();
	vmaDestroyAllocator(vram.allocator);
}

template <vk::ObjectType T, typename U>
void setDebugName(VKDevice const& device, U handle, char const* name) {
	if (device.flags.test(VKDevice::Flag::eDebugMsgr)) {
		static auto count = std::atomic<int>{};
		auto nameFallback = std::string{};
		name = getName(name, nameFallback, count);
		device.setDebugName(T, handle, name);
	}
}

UniqueImage Vram::makeImage(vk::ImageCreateInfo info, bool host, char const* name, bool linear) const {
	if (!allocator || !commandFactory) { return {}; }
	if (info.extent.width > deviceLimits.maxImageDimension2D || info.extent.height > deviceLimits.maxImageDimension2D) {
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

	setDebugName<vk::ObjectType::eImage>(device, ret, name);

	return VmaImage{{vk::Image(ret), allocator, handle}, info.initialLayout, info.extent, info.tiling, BlitCaps::make(device.gpu, info.format)};
}

UniqueBuffer Vram::makeBuffer(vk::BufferCreateInfo info, bool host, char const* name) const {
	if (!commandFactory || !allocator) { return {}; }
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

	setDebugName<vk::ObjectType::eBuffer>(device, ret, name);

	return VmaBuffer{{vk::Buffer(ret), allocator, handle}, info.size, map};
}

void Vram::blit(vk::CommandBuffer cmd, vk::Image in, vk::Image out, TRect<std::int32_t> inr, TRect<std::int32_t> outr, vk::Filter filter) {
	auto isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	auto const srcOffsets = std::array<vk::Offset3D, 2>{{{inr.offset.x, inr.offset.y}, {inr.extent.x, inr.extent.y, 1}}};
	auto const dstOffsets = std::array<vk::Offset3D, 2>{{{outr.offset.x, outr.offset.y}, {outr.extent.x, outr.extent.y, 1}}};
	auto ib = vk::ImageBlit(isrl, srcOffsets, isrl, dstOffsets);
	cmd.blitImage(in, vk::ImageLayout::eTransferSrcOptimal, out, vk::ImageLayout::eTransferDstOptimal, ib, filter);
}

template <typename Span, std::output_iterator<UniqueBuffer> Out>
static void copy(Vram const& vram, VmaBuffer dst, vk::CommandBuffer cmd, Span const& data, char const* name, Out out) {
	auto const size = sizeof(decltype(data[0])) * std::size(data);
	if (!dst.resource || size == 0) { return; }
	auto bci = vk::BufferCreateInfo({}, static_cast<vk::DeviceSize>(size), vk::BufferUsageFlagBits::eTransferSrc);
	auto str = ktl::kformat("{}_staging", name);
	auto stage = vram.makeBuffer(bci, true, str.c_str());
	assert(stage);
	stage->write(data.data(), data.size());
	cmd.copyBuffer(stage->resource, dst.resource, vk::BufferCopy({}, {}, bci.size));
	*out++ = std::move(stage);
}

bool ImageWriter::canBlit(VmaImage const& src, VmaImage const& dst) { return src.blitFlags().test(BlitFlag::eSrc) && dst.blitFlags().test(BlitFlag::eDst); }

bool ImageWriter::write(VmaImage& out, std::span<std::byte const> data, URegion region, vk::ImageLayout il) {
	auto bci = vk::BufferCreateInfo({}, data.size(), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst);
	auto buffer = vram->makeBuffer(bci, true, "image_copy_staging");
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
	if (!canBlit(in, out) || in.extent == out.extent) { return copy(in, out, inr, outr, il); }
	if (in.extent.width == 0 || in.extent.height == 0) { return false; }
	if (out.extent.width == 0 || out.extent.height == 0) { return false; }

	in.transition(cb, vk::ImageLayout::eTransferSrcOptimal);
	out.transition(cb, vk::ImageLayout::eTransferDstOptimal);
	Vram::blit(cb, in.resource, out.resource, inr, outr, filter);
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
} // namespace vf
