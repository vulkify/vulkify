#pragma once
#include <ktl/hash_table.hpp>
#include <ttf/ft.hpp>
#include <vulkify/core/ptr.hpp>
#include <vulkify/graphics/atlas.hpp>
#include <vulkify/ttf/character.hpp>
#include <vulkify/ttf/glyph.hpp>

namespace vf {
struct Vram;

class GfxFont {
  public:
	using Height = Glyph::Height;

	GfxFont() = default;
	GfxFont(Context const& context);

	explicit operator bool() const;

	Character get(Codepoint codepoint, Height height);

	struct Face {
		Vram const* vram{};
		FtUnique<FtFace> face{};
	};

	struct Entry {
		Glyph glyph{};
		QuadTexCoords coords{};
	};
	struct Font {
		Atlas atlas{};
		ktl::hash_table<Codepoint, Entry> map{};
	};

	Font& get_or_make(Height height);
	Entry& insert(Font& out_font, Codepoint codepoint, Atlas::Bulk* bulk);
	Ptr<Atlas const> atlas(Height height) const;
	Ptr<Texture const> texture(Height height) const;

	ktl::hash_table<Height, Font> m_fonts{};
	Face m_face{};
};
} // namespace vf
