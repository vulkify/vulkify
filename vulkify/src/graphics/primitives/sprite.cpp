#include <vulkify/context/context.hpp>
#include <vulkify/context/frame.hpp>
#include <vulkify/graphics/primitives/sprite.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
Sprite::Sprite(Context const& context, glm::vec2 size) : Prop(context) { set_size(size); }

Sprite& Sprite::set_size(glm::vec2 size) {
	m_state.size = size;
	m_buffer.write(Geometry::make_quad(m_state));
	return *this;
}

Sprite& Sprite::set_sheet(Ptr<Sheet const> sheet, UvIndex index) {
	if (sheet) {
		m_texture = sheet->texture();
		set_uv_rect(sheet->uv(index));
	} else {
		m_texture = {};
	}
	return *this;
}

void Sprite::draw(Surface const& surface, RenderState const& state) const {
	if (m_texture) {
		surface.draw(Drawable{m_instance, m_buffer, m_texture}, state);
	} else if (draw_invalid) {
		auto instance = m_instance;
		instance.tint = magenta_v;
		surface.draw(Drawable{instance, m_buffer}, state);
	}
}

Sprite& Sprite::set_uv_rect(UvRect uv) {
	m_state.uv = uv;
	m_buffer.write(Geometry::make_quad(m_state));
	return *this;
}

auto Sprite::Sheet::add_uv(UvRect uv) -> UvIndex {
	auto const ret = m_uvs.size();
	m_uvs.push_back(uv);
	return static_cast<UvIndex>(ret);
}

UvRect const& Sprite::Sheet::uv(UvIndex index) const {
	static constexpr auto default_v = UvRect{};
	if (m_uvs.empty()) { return default_v; }
	if (index >= m_uvs.size()) { index = 0; }
	return m_uvs[index];
}

auto Sprite::Sheet::set_uvs(ktl::not_null<Texture const*> texture, std::uint32_t const rows, std::uint32_t const columns, glm::uvec2 const pad) -> Sheet& {
	if (!*texture || rows == 0 || columns == 0) { return *this; }
	m_uvs.clear();
	m_texture = texture->handle();
	auto const extent = glm::vec2(texture->extent());
	auto const frame = glm::vec2(extent.x / static_cast<float>(columns), extent.y / static_cast<float>(rows));
	auto const padf = glm::vec2(pad);
	auto const tile = frame - 2.0f * padf;
	auto o = glm::vec2{};
	for (std::uint32_t row = 0; row < rows; ++row) {
		o.x = {};
		for (std::uint32_t column = 0; column < columns; ++column) {
			auto const tl = o + padf;
			auto const br = tl + tile - padf;
			add_uv(UvRect{.top_left = tl / extent, .bottom_right = br / extent});
			o.x += frame.x;
		}
		o.y += frame.y;
	}
	return *this;
}

auto Sprite::Sheet::set_texture(TextureHandle texture) -> Sheet& {
	m_texture = texture;
	return *this;
}
} // namespace vf
