#include <detail/gfx_buffer_image.hpp>
#include <detail/gfx_command_buffer.hpp>
#include <vulkify/graphics/atlas.hpp>

namespace vf {
namespace {
constexpr std::uint32_t pot(std::uint32_t const in) {
	if (in == 0) { return 0; }
	auto ret = std::uint32_t{1};
	while (ret < in) { ret <<= 1; }
	return ret;
}
} // namespace

Atlas::Atlas(GfxDevice const& device, Extent const initial, Rgba const rgba) : m_texture(device, {}) {
	if (m_texture.m_allocation) { m_texture.create(Bitmap(rgba, initial).image()); }
}

QuadTexCoords Atlas::add(Image::View const image) {
	if (!m_texture) { return {}; }
	if (image.extent.x == 0 || image.extent.y == 0 || image.data.empty()) { return {}; }

	auto cb = GfxCommandBuffer{m_texture.m_device};
	if (!prepare(cb, image.extent)) { return {}; }
	auto ret = insert(cb, image);

	return ret;
}

void Atlas::clear(Rgba const rgba) {
	if (!m_texture.m_allocation) { return; }
	auto* image = static_cast<GfxImage*>(m_texture.m_allocation.get());
	auto cb = GfxCommandBuffer{m_texture.m_device};
	cb.writer.clear(image->image.cache.image, rgba);
	m_state = {};
}

void Atlas::next_line() {
	m_state.head.x = pad_v.x;
	m_state.head.y += m_state.nextY;
	m_state.nextY = 0;
}

static constexpr bool in(glm::uvec2 extent, glm::uvec2 point) { return point.x < extent.x && point.y < extent.y; }

bool Atlas::prepare(GfxCommandBuffer& cb, Extent const extent) {
	if (auto end = m_state.head + extent + pad_v; in(m_texture.extent(), end)) { return true; }
	next_line();
	auto end = m_state.head + extent + pad_v;
	if (in(m_texture.extent(), end)) { return true; }
	auto const target = Extent(std::max(m_texture.extent().x, end.x), std::max(m_texture.extent().y, end.y));
	return resize(cb, target);
}

bool Atlas::resize(GfxCommandBuffer& cb, Extent const target) {
	if (!m_texture.m_device) { return false; }
	auto texture = Texture(*m_texture.m_device);
	auto* src = static_cast<GfxImage*>(m_texture.m_allocation.get());
	auto* dst = static_cast<GfxImage*>(texture.m_allocation.get());
	if (!src || !dst) { return false; }
	texture.refresh(*dst, {pot(target.x), pot(target.y)});
	static constexpr auto layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	auto rect = TRect<std::uint32_t>{m_texture.extent()};
	auto const ret = cb.writer.blit(src->image.cache.image.get(), dst->image.cache.image.get(), rect, rect, vk::Filter::eLinear, {layout, layout});
	if (ret) { m_texture = std::move(texture); }
	return ret;
}

bool Atlas::overwrite(GfxCommandBuffer& cb, Image::View image, Texture::Rect const& region) {
	if (static_cast<std::uint32_t>(region.offset.x) + region.extent.x > extent().x ||
		static_cast<std::uint32_t>(region.offset.y) + region.extent.y > extent().y) {
		return false;
	}
	if (!m_texture.m_allocation) { return false; }
	auto* self = static_cast<GfxImage*>(m_texture.m_allocation.get());
	if (!self) { return false; }
	return cb.writer.write(self->image.cache.image, image.data, region, vk::ImageLayout::eShaderReadOnlyOptimal);
}

QuadTexCoords Atlas::insert(GfxCommandBuffer& cb, Image::View image) {
	auto const rect = Texture::Rect{image.extent, m_state.head};
	auto res = overwrite(cb, image, rect);
	if (!res) { return {}; }

	auto const ret = QuadTexCoords{glm::vec2(m_state.head), glm::vec2(m_state.head + image.extent)};
	m_state.head.x += image.extent.x + pad_v.x;
	m_state.nextY = std::max(m_state.nextY, image.extent.y + pad_v.y);
	return ret;
}

Atlas::Bulk::Bulk(Atlas& atlas) : m_impl(ktl::make_unique<GfxCommandBuffer>(atlas.texture().m_device)), m_atlas(atlas) {}
Atlas::Bulk::~Bulk() = default;

QuadTexCoords Atlas::Bulk::add(Image::View image) {
	if (!m_atlas.m_texture) { return {}; }
	if (image.extent.x == 0 || image.extent.y == 0 || image.data.empty()) { return {}; }
	if (!m_atlas.prepare(*m_impl, image.extent)) { return {}; }
	return m_atlas.insert(*m_impl, image);
}
} // namespace vf
