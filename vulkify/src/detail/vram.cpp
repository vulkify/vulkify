#include <detail/command_pool.hpp>
#include <detail/trace.hpp>
#include <detail/vram.hpp>
#include <ktl/fixed_vector.hpp>
#include <atomic>

namespace vf {
namespace {
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

bool VmaBuffer::write(void const* data, std::size_t size) {
	if (size == full_v) { size = this->size; }
	if (!map || size == 0) { return false; }
	std::memcpy(map, data, size);
	return true;
}

void VmaBuffer::Deleter::operator()(VmaBuffer const& buffer) const {
	if (buffer.map) { vmaUnmapMemory(buffer.allocator, buffer.handle); }
	vmaDestroyBuffer(buffer.allocator, buffer.resource, buffer.handle);
}

void VmaImage::transition(vk::CommandBuffer cb, vk::ImageLayout to, ImageBarrier barrier) {
	if (layout == to || to == vk::ImageLayout::eUndefined) { return; }
	barrier.layouts = {layout, to};
	barrier(cb, resource);
	layout = to;
}

void VmaImage::Deleter::operator()(const VmaImage& image) const { vmaDestroyImage(image.allocator, image.resource, image.handle); }

UniqueVram UniqueVram::make(vk::Instance instance, VKDevice device) {
	VmaAllocatorCreateInfo allocatorInfo = {};
	auto dl = VULKAN_HPP_DEFAULT_DISPATCHER;
	allocatorInfo.instance = static_cast<VkInstance>(instance);
	allocatorInfo.device = static_cast<VkDevice>(device.device);
	allocatorInfo.physicalDevice = static_cast<VkPhysicalDevice>(device.gpu);
	VmaVulkanFunctions vkFunc = {};
	vkFunc.vkGetInstanceProcAddr = dl.vkGetInstanceProcAddr;
	vkFunc.vkGetDeviceProcAddr = dl.vkGetDeviceProcAddr;
	allocatorInfo.pVulkanFunctions = &vkFunc;
	auto vram = Vram{device};
	if (vmaCreateAllocator(&allocatorInfo, &vram.allocator) != VK_SUCCESS) {
		VF_TRACE("Failed to create Vram!");
		return {};
	}
	VF_TRACE("Vram constructed");
	auto factory = ktl::make_unique<CommandFactory>();
	factory->commandPools.factory().device = device;
	vram.commandFactory = factory.get();
	vram.maxAnisotropy = device.gpu.getProperties().limits.maxSamplerAnisotropy;
	return {std::move(factory), vram};
}

void Vram::Deleter::operator()(Vram const& vram) const {
	vram.device.defer.queue->entries.clear();
	vram.commandFactory->commandPools.clear();
	vmaDestroyAllocator(vram.allocator);
}

UniqueImage Vram::makeImage(vk::ImageCreateInfo info, bool host, char const* name, bool linear) const {
	if (!allocator || !commandFactory) { return {}; }
	info.tiling = linear ? vk::ImageTiling::eLinear : vk::ImageTiling::eOptimal;
	if (info.imageType == vk::ImageType()) { info.imageType = vk::ImageType::e2D; }
	if (info.mipLevels == 0U) { info.mipLevels = 1U; }
	if (info.arrayLayers == 0U) { info.arrayLayers = 1U; }
	info.sharingMode = vk::SharingMode::eExclusive;
	info.queueFamilyIndexCount = 1U;
	info.pQueueFamilyIndices = &device.queue.family;

	auto vaci = VmaAllocationCreateInfo{};
	vaci.usage = host ? VMA_MEMORY_USAGE_AUTO_PREFER_HOST : VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
	if (host) { vaci.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT; }
	auto const& imageInfo = static_cast<VkImageCreateInfo const&>(info);
	auto ret = VkImage{};
	auto handle = VmaAllocation{};
	if (auto res = vmaCreateImage(allocator, &imageInfo, &vaci, &ret, &handle, nullptr); res != VK_SUCCESS) { return {}; }

	if (device.flags.test(VKDevice::Flag::eDebugMsgr)) {
		static auto count = std::atomic<int>{};
		auto nameFallback = std::string{};
		name = getName(name, nameFallback, count);
		device.setDebugName(vk::ObjectType::eImage, ret, name);
	}

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

	if (device.flags.test(VKDevice::Flag::eDebugMsgr)) {
		static auto id = std::atomic<int>{};
		auto nameFallback = std::string{};
		name = getName(name, nameFallback, id);
		device.setDebugName(vk::ObjectType::eBuffer, ret, name);
	}

	return VmaBuffer{{vk::Buffer(ret), allocator, handle}, info.size, map};
}

template <typename Span, std::output_iterator<UniqueBuffer> Out>
static void copy(Vram const& vram, VmaBuffer dst, vk::CommandBuffer cmd, Span const& data, char const* name, Out out) {
	auto const size = sizeof(decltype(data[0])) * std::size(data);
	if (!dst.resource || size == 0) { return; }
	auto bci = vk::BufferCreateInfo({}, static_cast<vk::DeviceSize>(size), vk::BufferUsageFlagBits::eTransferSrc);
	auto str = ktl::str_format("{}_staging", name);
	auto stage = vram.makeBuffer(bci, true, str.c_str());
	assert(stage);
	stage->write(data.data(), data.size());
	cmd.copyBuffer(stage->resource, dst.resource, vk::BufferCopy({}, {}, bci.size));
	*out++ = std::move(stage);
}

BufferCache Vram::makeVIBuffer(Geometry const& geometry, BufferCache::Type type, char const* name) const {
	static constexpr auto s_vert = Vertex{};
	if (!device || !commandFactory) { return {}; }
	auto verts = std::span<Vertex const>(geometry.vertices);
	if (verts.empty()) { verts = {&s_vert, 1}; }
	auto ret = BufferCache{};
	ret.type = type;
	bool const gpuOnly = type == BufferCache::Type::eGpuOnly;
	bool const host = gpuOnly ? false : true;

	auto bci = vk::BufferCreateInfo{};
	bci.usage = vk::BufferUsageFlagBits::eVertexBuffer;
	if (gpuOnly) { bci.usage |= vk::BufferUsageFlagBits::eTransferDst; }
	static auto count = std::atomic<int>{};
	auto nameFallback = std::string{};
	name = getName(name, nameFallback, count);
	bci.size = verts.size_bytes();
	auto str = ktl::str_format("{}_vbo", name);
	ret.buffers.push_back(makeBuffer(bci, host, str.c_str()));
	if (!geometry.indices.empty()) {
		bci.usage = vk::BufferUsageFlagBits::eIndexBuffer;
		bci.size = geometry.indices.size() * sizeof(decltype(geometry.indices[0]));
		str = ktl::str_format("{}_ibo", name);
		ret.buffers.push_back(makeBuffer(bci, host, str.c_str()));
	}

	auto cmd = InstantCommand(commandFactory->get());
	auto scratch = ktl::fixed_vector<UniqueBuffer, 4>{};
	if (gpuOnly) {
		copy(*this, ret.buffers[0].get(), cmd.cmd, verts, name, std::back_inserter(scratch));
		if (ret.buffers.size() == 2) { copy(*this, ret.buffers[1].get(), cmd.cmd, geometry.indices, name, std::back_inserter(scratch)); }
	} else {
		ret.buffers[0]->write(verts.data());
		if (ret.buffers.size() == 2) { ret.buffers[1]->write(geometry.indices.data()); }
	}
	cmd.submit();

	return ret;
}

bool ImageWriter::canBlit(VmaImage const& src, VmaImage const& dst) { return src.blitFlags().test(BlitFlag::eSrc) && dst.blitFlags().test(BlitFlag::eDst); }

bool ImageWriter::write(VmaImage& out, std::span<std::byte const> data, glm::uvec2 extent, glm::ivec2 offset, vk::ImageLayout il) {
	auto bci = vk::BufferCreateInfo({}, data.size(), vk::BufferUsageFlagBits::eTransferSrc | vk::BufferUsageFlagBits::eTransferDst);
	auto buffer = vram.makeBuffer(bci, true, "image_copy_staging");
	if (!buffer || !buffer->write(data.data())) { return false; }

	if (extent.x == 0 && extent.y == 0) {
		if (offset.x != 0 || offset.y != 0) { return false; }
		extent = {out.extent.width, out.extent.height};
	}
	if (extent.x == 0 || extent.y == 0) { return false; }

	auto isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	auto icr = vk::ImageCopy(isrl, {}, isrl, {}, vk::Extent3D(extent.x, extent.y, 1));
	auto bic = vk::BufferImageCopy({}, {}, {}, isrl, {offset.x, offset.y, 0}, icr.extent);
	out.transition(cb, vk::ImageLayout::eTransferDstOptimal);
	cb.copyBufferToImage(buffer->resource, out.resource, out.layout, bic);
	out.transition(cb, il);
	scratch.push_back(std::move(buffer));

	return true;
}

bool ImageWriter::blit(VmaImage& in, VmaImage& out, vk::Filter filter, TPair<vk::ImageLayout> il) const {
	if (!canBlit(in, out) || in.extent == out.extent) { return copy(in, out, il); }
	if (in.extent.width == 0 || in.extent.height == 0) { return false; }
	if (out.extent.width == 0 || out.extent.height == 0) { return false; }

	auto isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	in.transition(cb, vk::ImageLayout::eTransferSrcOptimal);
	out.transition(cb, vk::ImageLayout::eTransferDstOptimal);
	auto srcOffset = vk::Offset3D(static_cast<int>(in.extent.width), static_cast<int>(in.extent.height), 1);
	auto dstOffset = vk::Offset3D(static_cast<int>(out.extent.width), static_cast<int>(out.extent.height), 1);
	auto ib = vk::ImageBlit(isrl, {vk::Offset3D(), srcOffset}, isrl, {vk::Offset3D(), dstOffset});
	cb.blitImage(in.resource, in.layout, out.resource, out.layout, ib, filter);
	in.transition(cb, il.first);
	out.transition(cb, il.second);

	return true;
}

bool ImageWriter::copy(VmaImage& in, VmaImage& out, TPair<vk::ImageLayout> il) const {
	if (in.extent.width == 0 || in.extent.height == 0) { return false; }
	if (out.extent.width == 0 || out.extent.height == 0) { return false; }

	auto isrl = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
	in.transition(cb, vk::ImageLayout::eTransferSrcOptimal);
	out.transition(cb, vk::ImageLayout::eTransferDstOptimal);
	auto ib = vk::ImageCopy(isrl, {}, isrl, {}, in.extent);
	cb.copyImage(in.resource, in.layout, out.resource, out.layout, ib);
	in.transition(cb, il.first);
	out.transition(cb, il.second);

	return true;
}
} // namespace vf
