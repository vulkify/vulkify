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

Atlas::Atlas(Context const& context, std::string name, Extent const initial) : m_texture(context, std::move(name), Bitmap(black_v, initial).image()) {}

Atlas::Id Atlas::add(Image::View const image) {
	if (!m_texture) { return {}; }
	if (image.extent.x == 0 || image.extent.y == 0 || image.bytes.empty()) { return {}; }
	if (!prepare(image.extent)) { return {}; }

	auto const rect = Texture::Rect{image.extent, m_state.head};
	auto res = m_texture.overwrite(image, rect);
	if (!res) { return {}; }

	auto const uv = UVRect{glm::vec2(m_state.head), glm::vec2(m_state.head + image.extent)};
	m_state.head.x += image.extent.x + pad_v.x;
	m_state.nextY = std::max(m_state.nextY, image.extent.y + pad_v.y);
	auto const id = ++m_state.next;
	m_uvMap.insert_or_assign(id, uv);
	return id;
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
	writer.clear(m_texture.m_allocation->image.cache.image, black_v);
	m_uvMap.clear();
	m_state = {};
}

void Atlas::nextLine() {
	m_state.head.x = pad_v.x;
	m_state.head.y += m_state.nextY;
	m_state.nextY = 0;
}

bool Atlas::prepare(Extent const extent) {
	auto overflow = [&] { return extent.x + m_state.head.x + pad_v.x > m_texture.extent().x; };
	auto area = m_state.head + extent + pad_v;
	if (overflow()) {
		// not enough width remaining, expand height, go to next line
		area.y += m_state.nextY;
		nextLine();
	}
	if (overflow()) {
		// insufficient total width, expand width
		area.x = extent.x + 2 * pad_v.x;
	}
	return reserve(area);
}

bool Atlas::reserve(Extent const target) {
	auto const current = m_texture.extent();
	if (current.y >= target.y && current.x >= target.x) { return true; }
	return resize(target);
}

bool Atlas::resize(Extent const target) {
	auto const& vram = m_texture.vram();
	auto texture = Texture(vram, m_texture.name(), {});
	texture.refresh({pot(target.x), pot(target.y)});
	static constexpr auto layout = vk::ImageLayout::eShaderReadOnlyOptimal;
	auto cmd = InstantCommand(vram.commandFactory->get());
	auto writer = ImageWriter{vram, cmd.cmd};
	auto rect = TRect<std::uint32_t>{m_texture.extent()};
	auto src = m_texture.m_allocation.get()->image.cache.image.get();
	auto dst = texture.m_allocation.get()->image.cache.image.get();
	auto const ret = writer.blit(src, dst, rect, rect, vk::Filter::eLinear, {layout, layout});
	cmd.submit();
	if (ret) { m_texture = std::move(texture); }
	return ret;
}
} // namespace vf
