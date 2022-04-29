#include <detail/shared_impl.hpp>
#include <detail/trace.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
namespace {
constexpr vk::SamplerAddressMode getMode(Texture::Mode const mode) {
	switch (mode) {
	case Texture::Mode::eRepeat: return vk::SamplerAddressMode::eRepeat;
	case Texture::Mode::eClampBorder: return vk::SamplerAddressMode::eClampToBorder;
	case Texture::Mode::eClampEdge: return vk::SamplerAddressMode::eClampToEdge;
	default: return vk::SamplerAddressMode::eClampToEdge;
	}
}

constexpr vk::Filter getFilter(Texture::Filter const filter) { return filter == Texture::Filter::eLinear ? vk::Filter::eLinear : vk::Filter::eNearest; }
} // namespace

Texture::Texture(Vram const& vram, std::string name, Bitmap bitmap, Mode mode, Filter filter)
	: GfxResource(vram, std::move(name)), m_mode(mode), m_filter(filter) {
	m_allocation->image.cache.setTexture(true);
	auto sci = samplerInfo(vram, getMode(mode), getFilter(filter));
	m_allocation->image.sampler = vram.device.device.createSamplerUnique(sci);
	if (!Bitmap::valid(bitmap)) {
		VF_TRACEF("[Texture:{}] Invalid bitmap", m_name);
		bitmap = Bitmap(white_v);
	}
	create(std::move(bitmap));
}

Result<void> Texture::create(Bitmap bitmap) {
	if (!Bitmap::valid(bitmap)) {
		VF_TRACEF("[Texture:{}] Invalid bitmap", m_name);
		return Error::eInvalidArgument;
	}
	if (!m_allocation->vram) { return Error::eInactiveInstance; }
	m_bitmap = std::move(bitmap);
	m_allocation->image.cache.refresh({m_bitmap.extent().x, m_bitmap.extent().y, 1});
	write(m_bitmap, {});
	return Result<void>::success();
}

Result<void> Texture::overwrite(Bitmap::View const bitmap, glm::ivec2 const offset) {
	if (!m_allocation->vram || !m_allocation->image.cache.image) { return Error::eInactiveInstance; }
	if (!m_bitmap.overwrite(bitmap, offset)) { return Error::eInvalidArgument; }
	write(bitmap, offset);
	return Result<void>::success();
}

void Texture::write(Bitmap::View const bitmap, glm::ivec2 const offset) {
	auto vec = std::vector<std::byte>();
	vec.reserve(bitmap.extent.x * bitmap.extent.y * 4);
	for (Rgba const pixel : bitmap.pixels) { rgbaToByte(pixel, std::back_inserter(vec)); }
	auto cmd = InstantCommand(m_allocation->vram.commandFactory->get());
	auto writer = ImageWriter{m_allocation->vram, cmd.cmd};
	auto const layouts = std::make_pair(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
	writer(m_allocation->image.cache.image, vec.data(), vec.size(), bitmap.extent, offset, &layouts);
	cmd.submit();
}

Texture Texture::clone(std::string name) const {
	if (!m_allocation->vram || !m_allocation->image.cache.image) { return {}; }
	auto ret = Texture(m_allocation->vram, std::move(name), m_bitmap, m_mode, m_filter);
	if (!m_allocation->image.cache.image) { return ret; }
	ret.create(m_bitmap);
	return ret;
}

Extent2D Texture::extent() const { return {m_allocation->image.cache.info.extent.width, m_allocation->image.cache.info.extent.height}; }
} // namespace vf
