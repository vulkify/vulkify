#include <vulkify/graphics/primitives/text.hpp>
#include <vulkify/graphics/surface.hpp>
#include <vulkify/ttf/scribe.hpp>
#include <cassert>

namespace vf {
Text::Text(Context const& context, std::string name) : m_gbo(context, std::move(name)) {}

Text::operator bool() const { return m_ttf && *m_ttf && m_gbo; }

Text& Text::setFont(ktl::not_null<Ttf*> ttf) {
	m_ttf = ttf;
	return *this;
}

void Text::draw(Surface const& surface) const {
	if (*this) { surface.draw(drawable()); }
}

void Text::update() const {
	assert(m_ttf);
	auto scribe = Scribe{*m_ttf};
	scribe.write(Scribe::Block{text}, pivot);
	m_gbo.write(std::move(scribe.geometry));
}

Drawable Text::drawable() const {
	update();
	return {{&m_instance, 1}, m_gbo, m_ttf->texture()};
}
} // namespace vf
