#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
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
} // namespace

Texture::Texture(Vram const& vram, std::string name, Bitmap bitmap, CreateInfo const& createInfo)
	: GfxResource(vram, std::move(name)), m_addressMode(createInfo.addressMode), m_filtering(createInfo.filtering) {
	if (!m_allocation || !m_allocation->vram) { return; }
	m_allocation->image.sampler = vram.device.device.createSamplerUnique(samplerInfo(vram, getMode(m_addressMode), getFilter(m_filtering)));
	m_allocation->image.cache.setTexture(true);
	if (!Bitmap::valid(bitmap)) {
		setInvalid();
		return;
	}
	create(std::move(bitmap));
}

Result<void> Texture::create(Bitmap bitmap) {
	if (!m_allocation || !m_allocation->vram) { return Error::eInactiveInstance; }
	if (!Bitmap::valid(bitmap)) {
		setInvalid();
		return Error::eInvalidArgument;
	}
	m_bitmap = std::move(bitmap);
	m_allocation->image.cache.refresh({m_bitmap.extent().x, m_bitmap.extent().y, 1});
	write(m_bitmap, {});
	return Result<void>::success();
}

Result<void> Texture::overwrite(Bitmap::View const bitmap, Extent2D const offset) {
	if (!m_allocation || !m_allocation->vram || !m_allocation->image.cache.image) { return Error::eInactiveInstance; }
	if (!m_bitmap.overwrite(bitmap, offset)) { return Error::eInvalidArgument; }
	write(bitmap, offset);
	return Result<void>::success();
}

Texture Texture::clone(std::string name) const {
	if (!m_allocation->vram || !m_allocation->image.cache.image) { return {}; }
	auto const createInfo = CreateInfo{m_addressMode, m_filtering};
	auto ret = Texture(m_allocation->vram, std::move(name), m_bitmap, createInfo);
	if (!m_allocation->image.cache.image) { return ret; }
	ret.create(m_bitmap);
	return ret;
}

Extent2D Texture::extent() const {
	if (!m_allocation) { return {}; }
	return {m_allocation->image.cache.info.extent.width, m_allocation->image.cache.info.extent.height};
}

void Texture::write(Bitmap::View const bitmap, Extent2D const offset) {
	auto vec = std::vector<std::byte>();
	vec.reserve(bitmap.extent.x * bitmap.extent.y * 4);
	for (Rgba const pixel : bitmap.pixels) { rgbaToByte(pixel, std::back_inserter(vec)); }
	auto cmd = InstantCommand(m_allocation->vram.commandFactory->get());
	auto writer = ImageWriter{m_allocation->vram, cmd.cmd};
	auto const layouts = std::make_pair(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	writer(m_allocation->image.cache.image, vec.data(), vec.size(), bitmap.extent, offset, &layouts);
	cmd.submit();
}

void Texture::setInvalid() {
	VF_TRACEF("[Texture:{}] Invalid bitmap", m_allocation->name);
	m_bitmap = Bitmap(magenta_v);
	m_allocation->image.cache.refresh({1, 1, 1});
	write(m_bitmap, {});
}
} // namespace vf
