#pragma once
#include <detail/gfx_allocation.hpp>
#include <ktl/hash_table.hpp>
#include <ttf/ft.hpp>
#include <vulkify/core/ptr.hpp>
#include <vulkify/graphics/atlas.hpp>
#include <vulkify/ttf/character.hpp>
#include <vulkify/ttf/glyph.hpp>

namespace vf {
struct GfxDevice;

class GfxFont : public GfxAllocation {
  public:
	using Height = Glyph::Height;

	GfxFont(GfxDevice const* device);

	explicit operator bool() const;

	Character get(Codepoint codepoint, Height height);

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

	ktl::hash_table<Height, Font> fonts{};
	FtUnique<FtFace> face{};
};
} // namespace vf
