#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <vulkify/context/context.hpp>
#include <vulkify/core/float_eq.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
namespace {
constexpr vk::SamplerAddressMode getMode(AddressMode const mode) {
	switch (mode) {
	case AddressMode::eRepeat: return vk::SamplerAddressMode::eRepeat;
	case AddressMode::eClampBorder: return vk::SamplerAddressMode::eClampToBorder;
	case AddressMode::eClampEdge: return vk::SamplerAddressMode::eClampToEdge;
	default: return vk::SamplerAddressMode::eClampToEdge;
	}
}

constexpr vk::Filter getFilter(Filtering const filtering) { return filtering == Filtering::eLinear ? vk::Filter::eLinear : vk::Filter::eNearest; }

constexpr std::array<std::byte, Image::channels_v> rgbaBytes(Rgba rgba) {
	auto ret = std::array<std::byte, Image::channels_v>{};
	Bitmap::rgbaToByte(rgba, ret.data());
	return ret;
}

void blit(ImageCache& in_cache, ImageCache& out_cache, Filtering filtering) {
	static constexpr auto layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	auto cmd = InstantCommand(in_cache.info.vram.commandFactory->get());
	auto writer = ImageWriter{in_cache.info.vram, cmd.cmd};
	auto inr = TRect<std::uint32_t>{{in_cache.image->extent.width, in_cache.image->extent.height}};
	auto outr = TRect<std::uint32_t>{{out_cache.image->extent.width, out_cache.image->extent.height}};
	writer.blit(in_cache.image, out_cache.image, inr, outr, getFilter(filtering), {layout, layout});
	cmd.submit();
}
} // namespace

Texture::Texture(Context const& context, std::string name, Image::View image, CreateInfo const& createInfo)
	: Texture(context.vram(), std::move(name), createInfo) {
	if (!m_allocation || !m_allocation->vram) { return; }
	create(image);
}

Result<void> Texture::create(Image::View image) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }
	if (!Image::valid(image)) {
		setInvalid();
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
	image.refresh({ext.x, ext.y, 1});
	if (!image.image) { return Error::eMemoryError; }

	blit(m_allocation->image.cache, image, m_filtering);
	m_allocation->replace(std::move(image));
	return Result<void>::success();
}

Texture Texture::clone(std::string name) const {
	if (!m_allocation || !m_allocation->vram || !m_allocation->image.cache.image) { return {}; }

	auto ret = Texture(m_allocation->vram, std::move(name), {m_addressMode, m_filtering});
	if (!ret.m_allocation) { return ret; }

	auto const ext = extent();
	if (ext.x == 0 || ext.y == 0) { return ret; }
	ret.refresh(ext);
	if (!ret.m_allocation->image.cache.image) { return ret; }

	blit(m_allocation->image.cache, ret.m_allocation->image.cache, m_filtering);
	return ret;
}

Extent Texture::extent() const {
	if (!m_allocation) { return {}; }
	return {m_allocation->image.cache.info.info.extent.width, m_allocation->image.cache.info.info.extent.height};
}

Texture::Texture(Vram const& vram, std::string name, CreateInfo const& createInfo)
	: GfxResource(vram, std::move(name)), m_addressMode(createInfo.addressMode), m_filtering(createInfo.filtering) {
	if (!m_allocation || !m_allocation->vram) { return; }
	m_allocation->image.sampler = vram.device.device.createSamplerUnique(samplerInfo(vram, getMode(m_addressMode), getFilter(m_filtering)));
	m_allocation->image.cache.setTexture(true);
}

void Texture::refresh(Extent extent) {
	auto const ext = vk::Extent3D(extent.x, extent.y, 1);
	auto const format = m_allocation->image.cache.info.info.format;
	if (!m_allocation->image.cache.ready(ext, format)) {
		auto cache = ImageCache{m_allocation->image.cache.info};
		cache.refresh(ext);
		m_allocation->replace(std::move(cache));
	}
}

void Texture::write(Image::View const image, Rect const& region) {
	auto cmd = InstantCommand(m_allocation->vram.commandFactory->get());
	auto writer = ImageWriter{m_allocation->vram, cmd.cmd};
	writer.write(m_allocation->image.cache.image, image.bytes, region, vk::ImageLayout::eShaderReadOnlyOptimal);
	cmd.submit();
}

void Texture::setInvalid() {
	VF_TRACEF("[vf::Texture:{}] Invalid bitmap", m_allocation->name);
	static constexpr auto magenta_bytes_v = rgbaBytes(magenta_v);
	m_allocation->image.cache.refresh({1, 1, 1});
	write({magenta_bytes_v, {1, 1}}, {{1, 1}});
}
} // namespace vf
