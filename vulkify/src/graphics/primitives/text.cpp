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

Text::Text(Context const& context, std::string name) : m_mesh(context, std::move(name)) {}

Text::operator bool() const { return m_ttf && *m_ttf && m_mesh; }

Text& Text::setFont(ktl::not_null<Ttf*> ttf) {
	m_ttf = ttf;
	return *this;
}

void Text::draw(Surface const& surface, Pipeline const& pipeline) const {
	update();
	surface.draw(m_mesh.drawable(), pipeline);
}

void Text::update() const {
	assert(m_ttf);
	auto scribe = Scribe{*m_ttf, height};
	scribe.write(Scribe::Block{text}, pivot(align));
	if (auto const* texture = m_ttf->texture(height)) {
		m_mesh.texture = texture->handle();
		m_mesh.gbo.write(std::move(scribe.geometry));
	}
}
} // namespace vf
