#include <detail/gfx_allocation.hpp>
#include <detail/gfx_font.hpp>
#include <vulkify/graphics/primitives/text.hpp>
#include <vulkify/graphics/surface.hpp>
#include <vulkify/ttf/scribe.hpp>
#include <cassert>

namespace vf {
namespace {
constexpr Scribe::Pivot pivot(Text::Align align) {
	auto ret = Scribe::Pivot{};
	switch (align.horz) {
	case Text::Horz::eLeft: ret.x = -0.5f; break;
	case Text::Horz::eRight: ret.x = 0.5f; break;
	default: break;
	}
	switch (align.vert) {
	case Text::Vert::eDown: ret.y = -0.5f; break;
	case Text::Vert::eUp: ret.y = 0.5f; break;
	default: break;
	}
	return ret;
}
} // namespace

Text::Text(GfxDevice const& device) { m_mesh.get() = vf::Mesh{device}; }

Text::operator bool() const { return m_ttf && m_ttf.allocation && m_mesh.get(); }

Text& Text::set_ttf(ktl::not_null<Ttf*> ttf) {
	if (*ttf) { return set_ttf(ttf->handle()); }
	return *this;
}

Text& Text::set_ttf(Handle<Ttf> ttf) {
	m_ttf = ttf;
	m_mesh.set_dirty();
	return *this;
}

Text& Text::set_string(std::string string) {
	m_text = std::move(string);
	m_mesh.set_dirty();
	return *this;
}

Text& Text::append(std::string string) {
	m_text += std::move(string);
	m_mesh.set_dirty();
	return *this;
}

Text& Text::append(char ch) {
	m_text += ch;
	m_mesh.set_dirty();
	return *this;
}

Text& Text::set_align(Align align) {
	m_align = align;
	m_mesh.set_dirty();
	return *this;
}

Text& Text::set_height(Height height) {
	m_height = height;
	m_mesh.set_dirty();
	return *this;
}

void Text::draw(Surface const& surface, RenderState const& state) const {
	if (m_mesh.dirty) { rebuild(); }
	surface.draw(m_mesh.get().drawable(), state);
}

void Text::rebuild() const {
	auto* self = static_cast<GfxFont*>(m_ttf.allocation);
	if (m_text.empty() || !self || !m_mesh.t) { return; }
	assert(self->type() == GfxAllocation::Type::eFont);
	auto scribe = Scribe{*self, m_height};
	scribe.write(Scribe::Block{m_text}, pivot(m_align));
	if (auto const* texture = self->texture(m_height)) {
		m_mesh.get().texture = texture->handle();
		m_mesh.get().buffer.write(std::move(scribe.geometry));
		m_mesh.set_clean();
	}
}
} // namespace vf
