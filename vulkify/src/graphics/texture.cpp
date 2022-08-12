#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <vulkify/context/context.hpp>
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
	auto cb = GfxCommandBuffer(in_cache.info.vram);
	auto inr = TRect<std::uint32_t>{{in_cache.image->extent.width, in_cache.image->extent.height}};
	auto outr = TRect<std::uint32_t>{{out_cache.image->extent.width, out_cache.image->extent.height}};
	cb.writer.blit(in_cache.image, out_cache.image, inr, outr, get_filter(filtering), {layout, layout});
}
} // namespace

Texture::Texture(Context const& context, Image::View image, CreateInfo const& createInfo) : Texture(context.vram(), createInfo) {
	if (!m_allocation || !m_allocation->vram) { return; }
	create(image);
}

Result<void> Texture::create(Image::View image) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }
	if (!Image::valid(image)) {
		set_invalid();
		return Error::eInvalidArgument;
	}

	refresh(image.extent);
	write(image, {image.extent});
	return Result<void>::success();
}

Result<void> Texture::overwrite(Image::View const image, Rect const& region) {
	if (!m_allocation || !m_allocation->vram || !m_allocation->image.cache.image) { return Error::eInactiveInstance; }
	if (static_cast<std::uint32_t>(region.offset.x) + region.extent.x > extent().x ||
		static_cast<std::uint32_t>(region.offset.y) + region.extent.y > extent().y) {
		return Error::eInvalidArgument;
	}

	write(image, region);
	return Result<void>::success();
}

Result<void> Texture::rescale(float scale) {
	if (!m_allocation || !m_allocation->vram || !m_allocation->image.cache.image) { return Error::eInactiveInstance; }
	if (FloatEq{}(scale, 1.0f)) { return Result<void>::success(); }
	auto const ext = Extent(glm::vec2(extent()) * scale);
	if (ext.x == 0 || ext.y == 0) { return Error::eInvalidArgument; }

	auto image = ImageCache{m_allocation->image.cache.info};
	image.refresh(ext);
	if (!image.image) { return Error::eMemoryError; }

	blit(m_allocation->image.cache, image, m_filtering);
	m_allocation->replace(std::move(image));
	return Result<void>::success();
}

Texture Texture::clone() const {
	auto ret = clone_image();
	if (!ret.m_allocation->image.cache.image) { return ret; }

	blit(m_allocation->image.cache, ret.m_allocation->image.cache, m_filtering);
	return ret;
}

Extent Texture::extent() const {
	if (!m_allocation) { return {}; }
	return {m_allocation->image.cache.info.info.extent.width, m_allocation->image.cache.info.info.extent.height};
}

TextureHandle Texture::handle() const { return {m_allocation.get()}; }

Texture::Texture(Vram const& vram, CreateInfo const& createInfo)
	: GfxResource(vram), m_address_mode(createInfo.address_mode), m_filtering(createInfo.filtering) {
	if (!m_allocation || !m_allocation->vram) { return; }
	m_allocation->image.sampler = vram.device.device.createSamplerUnique(sampler_info(vram, get_mode(m_address_mode), get_filter(m_filtering)));
	m_allocation->image.cache.set_texture(true);
	m_allocation->image.cache.info.info.format = get_format(createInfo.format);
}

Texture Texture::clone_image() const {
	if (!m_allocation || !m_allocation->vram || !m_allocation->image.cache.image) { return {}; }

	auto ret = Texture(m_allocation->vram, {m_address_mode, m_filtering});
	if (!ret.m_allocation) { return ret; }

	auto const ext = extent();
	if (ext.x == 0 || ext.y == 0) { return ret; }
	ret.refresh(ext);
	return ret;
}

void Texture::refresh(Extent extent) {
	auto const format = m_allocation->image.cache.info.info.format;
	if (!m_allocation->image.cache.ready(extent, format)) {
		auto cache = ImageCache{m_allocation->image.cache.info};
		cache.refresh(extent);
		m_allocation->replace(std::move(cache));
	}
}

void Texture::write(Image::View const image, Rect const& region) {
	auto cb = GfxCommandBuffer(m_allocation->vram);
	cb.writer.write(m_allocation->image.cache.image, image.data, region, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void Texture::set_invalid() {
	VF_TRACE("vf::Texture", vf::trace::Type::eWarn, "Invalid bitmap");
	static constexpr auto magenta_bytes_v = rgba_bytes(magenta_v);
	m_allocation->image.cache.refresh({1, 1});
	write({magenta_bytes_v, {1, 1}}, {{1, 1}});
}
} // namespace vf

namespace vf::refactor {
/*
Texture::Texture(Context const& context, Image::View image, CreateInfo const& createInfo) : Texture(context.device(), createInfo) {
	if (!m_handle) { return; }
	create(image);
}

Result<void> Texture::create(Image::View image) {
	if (!m_handle) { return Error::eInactiveInstance; }
	if (!Image::valid(image)) {
		set_invalid();
		return Error::eInvalidArgument;
	}

	refresh(image.extent);
	write(image, {image.extent});
	return Result<void>::success();
}

Result<void> Texture::overwrite(Image::View const image, Rect const& region) {
	if (!m_handle) { return Error::eInactiveInstance; }
	if (static_cast<std::uint32_t>(region.offset.x) + region.extent.x > extent().x ||
		static_cast<std::uint32_t>(region.offset.y) + region.extent.y > extent().y) {
		return Error::eInvalidArgument;
	}

	write(image, region);
	return Result<void>::success();
}

Result<void> Texture::rescale(float scale) {
	if (!m_handle) { return Error::eInactiveInstance; }
	if (FloatEq{}(scale, 1.0f)) { return Result<void>::success(); }
	auto const ext = Extent(glm::vec2(extent()) * scale);
	if (ext.x == 0 || ext.y == 0) { return Error::eInvalidArgument; }

	auto const* alloc =

		auto image = ImageCache{m_allocation->image.cache.info};
	image.refresh(ext);
	if (!image.image) { return Error::eMemoryError; }

	blit(m_allocation->image.cache, image, m_filtering);
	m_allocation->replace(std::move(image));
	return Result<void>::success();
}

Texture Texture::clone() const {
	auto ret = clone_image();
	if (!ret.m_allocation->image.cache.image) { return ret; }

	blit(m_allocation->image.cache, ret.m_allocation->image.cache, m_filtering);
	return ret;
}

Extent Texture::extent() const {
	if (!m_allocation) { return {}; }
	return {m_allocation->image.cache.info.info.extent.width, m_allocation->image.cache.info.info.extent.height};
}

TextureHandle Texture::handle() const { return {m_allocation.get()}; }

Texture::Texture(GfxDevice const& device, CreateInfo const& createInfo)
	: GfxResource(device), m_address_mode(createInfo.address_mode), m_filtering(createInfo.filtering) {
	if (!m_allocation || !m_allocation->vram) { return; }
	m_allocation->image.sampler = device.device.device.createSamplerUnique(sampler_info(device, get_mode(m_address_mode), get_filter(m_filtering)));
	m_allocation->image.cache.set_texture(true);
	m_allocation->image.cache.info.info.format = get_format(createInfo.format);
}

Texture Texture::clone_image() const {
	if (!m_allocation || !m_allocation->vram || !m_allocation->image.cache.image) { return {}; }

	auto ret = Texture(m_allocation->vram, {m_address_mode, m_filtering});
	if (!ret.m_allocation) { return ret; }

	auto const ext = extent();
	if (ext.x == 0 || ext.y == 0) { return ret; }
	ret.refresh(ext);
	return ret;
}

void Texture::refresh(Extent extent) {
	auto const format = m_allocation->image.cache.info.info.format;
	if (!m_allocation->image.cache.ready(extent, format)) {
		auto cache = ImageCache{m_allocation->image.cache.info};
		cache.refresh(extent);
		m_allocation->replace(std::move(cache));
	}
}

void Texture::write(Image::View const image, Rect const& region) {
	auto cb = GfxCommandBuffer(m_allocation->vram);
	cb.writer.write(m_allocation->image.cache.image, image.data, region, vk::ImageLayout::eShaderReadOnlyOptimal);
}

void Texture::set_invalid() {
	VF_TRACE("vf::Texture", vf::trace::Type::eWarn, "Invalid bitmap");
	static constexpr auto magenta_bytes_v = rgba_bytes(magenta_v);
	m_allocation->image.cache.refresh({1, 1});
	write({magenta_bytes_v, {1, 1}}, {{1, 1}});
}*/
} // namespace vf::refactor
