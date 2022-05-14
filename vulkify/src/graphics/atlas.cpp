#include <detail/shared_impl.hpp>
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

Atlas::Atlas(Context const& context, std::string name, Extent const initial) : m_texture(context, std::move(name), Bitmap({}, initial).image()) {}

Atlas::Id Atlas::add(Image::View const image) {
	if (!m_texture) { return {}; }
	if (image.extent.x == 0 || image.extent.y == 0 || image.data.empty()) { return {}; }

	auto cmd = InstantCommand{m_texture.vram().commandFactory->get()};
	auto writer = ImageWriter{m_texture.vram(), cmd.cmd};
	if (!prepare(writer, image.extent)) { return {}; }
	auto ret = insert(writer, image);
	cmd.submit();

	return ret;
}

UVRect Atlas::get(Id const id, UVRect const& fallback) const {
	if (!m_texture || m_texture.extent().x == 0 || m_texture.extent().y == 0) { return fallback; }
	auto const it = m_uvMap.find(id);
	if (it == m_uvMap.end()) { return fallback; }
	auto const extent = glm::vec2(m_texture.extent());
	return {it->second.topLeft / extent, it->second.bottomRight / extent};
}

void Atlas::clear() {
	if (!m_texture) { return; }
	auto cmd = InstantCommand(m_texture.vram().commandFactory->get());
	auto writer = ImageWriter{m_texture.vram(), cmd.cmd};
	writer.clear(m_texture.m_allocation->image.cache.image, {});
	m_uvMap.clear();
	m_state = {};
}

void Atlas::nextLine() {
	m_state.head.x = pad_v.x;
	m_state.head.y += m_state.nextY;
	m_state.nextY = 0;
}

static constexpr bool in(glm::uvec2 extent, glm::uvec2 point) { return point.x < extent.x && point.y < extent.y; }

bool Atlas::prepare(ImageWriter& writer, Extent const extent) {
	if (auto end = m_state.head + extent + pad_v; in(m_texture.extent(), end)) { return true; }
	nextLine();
	auto end = m_state.head + extent + pad_v;
	if (in(m_texture.extent(), end)) { return true; }
	auto const target = Extent(std::max(m_texture.extent().x, end.x), std::max(m_texture.extent().y, end.y));
	return resize(writer, target);
}

bool Atlas::resize(ImageWriter& writer, Extent const target) {
	auto const& vram = m_texture.vram();
	auto texture = Texture(vram, m_texture.name(), {});
	texture.refresh({pot(target.x), pot(target.y)});
	static constexpr auto layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	auto rect = TRect<std::uint32_t>{m_texture.extent()};
	auto src = m_texture.m_allocation.get()->image.cache.image.get();
	auto dst = texture.m_allocation.get()->image.cache.image.get();
	auto const ret = writer.blit(src, dst, rect, rect, vk::Filter::eLinear, {layout, layout});
	if (ret) { m_texture = std::move(texture); }
	return ret;
}

bool Atlas::overwrite(ImageWriter& writer, Image::View image, Texture::Rect const& region) {
	if (static_cast<std::uint32_t>(region.offset.x) + region.extent.x > extent().x ||
		static_cast<std::uint32_t>(region.offset.y) + region.extent.y > extent().y) {
		return false;
	}
	return writer.write(m_texture.m_allocation->image.cache.image, image.data, region, vk::ImageLayout::eShaderReadOnlyOptimal);
}

Atlas::Id Atlas::insert(ImageWriter& writer, Image::View image) {
	auto const rect = Texture::Rect{image.extent, m_state.head};
	auto res = overwrite(writer, image, rect);
	if (!res) { return {}; }

	auto const uv = UVRect{glm::vec2(m_state.head), glm::vec2(m_state.head + image.extent)};
	m_state.head.x += image.extent.x + pad_v.x;
	m_state.nextY = std::max(m_state.nextY, image.extent.y + pad_v.y);
	auto const id = ++m_state.next;
	m_uvMap.insert_or_assign(id, uv);
	return id;
}

struct Atlas::Bulk::Impl {
	InstantCommand cmd;
	ImageWriter writer;

	Impl(Atlas& atlas) : cmd(atlas.m_texture.vram().commandFactory->get()), writer{atlas.m_texture.vram(), cmd.cmd} {}
};

Atlas::Bulk::Bulk(Atlas& atlas) : m_impl(ktl::make_unique<Impl>(atlas)), m_atlas(atlas) {}
Atlas::Bulk::~Bulk() { m_impl->cmd.submit(); }

Atlas::Id Atlas::Bulk::add(Image::View image) {
	if (!m_atlas.m_texture) { return {}; }
	if (image.extent.x == 0 || image.extent.y == 0 || image.data.empty()) { return {}; }
	if (!m_atlas.prepare(m_impl->writer, image.extent)) { return {}; }
	return m_atlas.insert(m_impl->writer, image);
}
} // namespace vf
