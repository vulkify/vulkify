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

Text::Text(Context const& context, std::string name) { m_mesh.get() = {context, std::move(name)}; }

Text::operator bool() const { return m_ttf && *m_ttf && m_mesh.get(); }

Text& Text::setFont(ktl::not_null<Ttf*> ttf) {
	m_ttf = ttf;
	m_mesh.setDirty();
	return *this;
}

Text& Text::setString(std::string string) {
	m_text = std::move(string);
	m_mesh.setDirty();
	return *this;
}

Text& Text::append(std::string string) {
	m_text += std::move(string);
	m_mesh.setDirty();
	return *this;
}

Text& Text::append(char ch) {
	m_text += ch;
	m_mesh.setDirty();
	return *this;
}

Text& Text::setAlign(Align align) {
	m_align = align;
	m_mesh.setDirty();
	return *this;
}

Text& Text::setHeight(Height height) {
	m_height = height;
	m_mesh.setDirty();
	return *this;
}

void Text::draw(Surface const& surface, RenderState const& state) const {
	if (m_text.empty() || !m_ttf) { return; }
	if (m_mesh.dirty) { rebuild(); }
	surface.draw(m_mesh.get().drawable(), state);
}

void Text::rebuild() const {
	auto scribe = Scribe{*m_ttf, m_height};
	scribe.write(Scribe::Block{m_text}, pivot(m_align));
	if (auto const* texture = m_ttf->texture(m_height)) {
		m_mesh.get().texture = texture->handle();
		m_mesh.get().gbo.write(std::move(scribe.geometry));
		m_mesh.setClean();
	}
}
} // namespace vf
