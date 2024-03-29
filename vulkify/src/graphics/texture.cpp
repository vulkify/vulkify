#include <detail/gfx_allocations.hpp>
#include <detail/gfx_command_buffer.hpp>
#include <detail/gfx_device.hpp>
#include <detail/trace.hpp>
#include <vulkify/core/float_eq.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
namespace {
constexpr vk::SamplerAddressMode get_mode(AddressMode const mode) {
	switch (mode) {
	case AddressMode::eRepeat: return vk::SamplerAddressMode::eRepeat;
	case AddressMode::eClampBorder: return vk::SamplerAddressMode::eClampToBorder;
	case AddressMode::eClampEdge: return vk::SamplerAddressMode::eClampToEdge;
	default: return vk::SamplerAddressMode::eClampToEdge;
	}
}

constexpr vk::Filter get_filter(Filtering const filtering) { return filtering == Filtering::eLinear ? vk::Filter::eLinear : vk::Filter::eNearest; }

constexpr vk::Format get_format(ImageFormat const format) { return format == ImageFormat::eLinear ? vk::Format::eR8G8B8A8Unorm : vk::Format::eR8G8B8A8Srgb; }
constexpr std::array<std::byte, Image::channels_v> rgba_bytes(Rgba rgba) {
	auto ret = std::array<std::byte, Image::channels_v>{};
	Bitmap::rgba_to_byte(rgba, ret.data());
	return ret;
}

void blit(ImageCache& in_cache, ImageCache& out_cache, Filtering filtering) {
	static constexpr auto layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	auto cb = GfxCommandBuffer{in_cache.device};
	auto inr = TRect<std::uint32_t>{{in_cache.image->extent.width, in_cache.image->extent.height}};
	auto outr = TRect<std::uint32_t>{{out_cache.image->extent.width, out_cache.image->extent.height}};
	cb.writer.blit(in_cache.image, out_cache.image, inr, outr, get_filter(filtering), {layout, layout});
}
} // namespace

Texture::Texture(GfxDevice const& device, Image::View image, CreateInfo const& createInfo)
	: GfxDeferred(&device), m_address_mode(createInfo.address_mode), m_filtering(createInfo.filtering) {
	if (!device) { return; }
	auto gfx_image = ktl::make_unique<GfxImage>(&device);
	gfx_image->image.sampler = device.device.device.createSamplerUnique(device.sampler_info(get_mode(m_address_mode), get_filter(m_filtering)));
	gfx_image->image.cache.set_texture(true);
	gfx_image->image.cache.info.info.format = get_format(createInfo.format);
	m_allocation = std::move(gfx_image);

	static constexpr auto white_byte_v = static_cast<std::byte>(0xff);
	static constexpr std::byte white_v[] = {white_byte_v, white_byte_v, white_byte_v, white_byte_v};
	if (image.data.empty()) { image = {white_v, {1, 1}}; }
	create(image);
}

Result<void> Texture::create(Image::View image) {
	if (!m_allocation || !m_allocation->device()) { return Error::eInactiveInstance; }
	assert(m_allocation->type() == GfxAllocation::Type::eImage);
	auto* self = static_cast<GfxImage*>(m_allocation.get());
	if (!Image::valid(image)) {
		set_invalid(*self);
		return Error::eInvalidArgument;
	}

	refresh(*self, image.extent);
	write(*self, image, {image.extent});
	return Result<void>::success();
}

Result<void> Texture::overwrite(Image::View const image, Rect const& region) {
	auto* self = static_cast<GfxImage*>(m_allocation.get());
	if (!self || !self->device()) { return Error::eInactiveInstance; }
	assert(self->type() == GfxAllocation::Type::eImage);
	if (static_cast<std::uint32_t>(region.offset.x) + region.extent.x > extent().x ||
		static_cast<std::uint32_t>(region.offset.y) + region.extent.y > extent().y) {
		return Error::eInvalidArgument;
	}

	write(*self, image, region);
	return Result<void>::success();
}

Result<void> Texture::rescale(float scale) {
	if (FloatEq{}(scale, 1.0f)) { return Result<void>::success(); }
	auto const ext = Extent(glm::vec2(extent()) * scale);
	if (ext.x == 0 || ext.y == 0) { return Error::eInvalidArgument; }

	auto* self = static_cast<GfxImage*>(m_allocation.get());
	if (!self || !self->device()) { return Error::eInactiveInstance; }
	assert(self->type() == GfxAllocation::Type::eImage);
	if (!self) { return Error::eInactiveInstance; }

	auto image = ImageCache{self->image.cache.info};
	image.refresh(ext);
	if (!image.image) { return Error::eMemoryError; }

	blit(self->image.cache, image, m_filtering);
	self->replace(std::move(image));
	return Result<void>::success();
}

Texture Texture::clone() const {
	auto* self = static_cast<GfxImage*>(m_allocation.get());
	if (!self || !self->device()) { return {}; }
	assert(self->type() == GfxAllocation::Type::eImage);

	auto ret = clone_image(*self);
	auto* other = static_cast<GfxImage*>(ret.m_allocation.get());
	if (!other) { return ret; }
	assert(other->type() == GfxAllocation::Type::eImage);

	if (!other) { return ret; }

	blit(self->image.cache, other->image.cache, m_filtering);
	return ret;
}

Extent Texture::extent() const {
	if (!m_allocation || !m_allocation->device()) { return {}; }
	assert(m_allocation->type() == GfxAllocation::Type::eImage);
	auto* self = static_cast<GfxImage*>(m_allocation.get());
	return {self->image.cache.info.info.extent.width, self->image.cache.info.info.extent.height};
}

Handle<Texture> Texture::handle() const { return {m_allocation.get()}; }

Texture Texture::clone_image(GfxImage& out_image) const {
	if (!m_device) { return {}; }
	auto ret = Texture{*m_device, {}, {m_address_mode, m_filtering}};
	if (!ret.m_allocation) { return ret; }

	auto const ext = extent();
	if (ext.x == 0 || ext.y == 0) { return ret; }
	ret.refresh(out_image, ext);
	return ret;
}

void Texture::refresh(GfxImage& out_image, Extent extent) {
	auto const format = out_image.image.cache.info.info.format;
	if (!out_image.image.cache.ready(extent, format)) {
		auto cache = ImageCache{.info = out_image.image.cache.info, .device = out_image.device()};
		cache.refresh(extent);
		out_image.replace(std::move(cache));
	}
}

void Texture::write(GfxImage& out_image, Image::View const image, Rect const& region) {
	auto cb = GfxCommandBuffer{m_device};
	cb.writer.write(out_image.image.cache.image, image.data, region, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void Texture::set_invalid(GfxImage& out_image) {
	VF_TRACE("vf::Texture", vf::trace::Type::eWarn, "Invalid bitmap");
	static constexpr auto magenta_bytes_v = rgba_bytes(magenta_v);
	out_image.image.cache.refresh({1, 1});
	write(out_image, {magenta_bytes_v, {1, 1}}, {{1, 1}});
}
} // namespace vf
