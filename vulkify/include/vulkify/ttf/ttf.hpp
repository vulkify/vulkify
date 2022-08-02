#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <ktl/hash_table.hpp>
#include <vulkify/core/ptr.hpp>
#include <vulkify/graphics/atlas.hpp>
#include <vulkify/ttf/glyph.hpp>

namespace vf {
///
/// \brief TrueType Font
///
class Ttf {
  public:
	using Height = Glyph::Height;
	static constexpr auto height_v = Glyph::height_v;

	struct Character {
		Ptr<Glyph const> glyph{};
		UvRect uv{};

		explicit operator bool() const { return glyph && *glyph; }
	};

	Ttf() noexcept;
	Ttf(Ttf&&) noexcept;
	Ttf& operator=(Ttf&&) noexcept;
	~Ttf() noexcept;

	Ttf(Context const& context, std::string name);
	explicit operator bool() const;

	bool load(std::span<std::byte const> bytes);
	bool load(char const* path);

	bool contains(Codepoint codepoint, Height height = height_v) const;
	Character find(Codepoint codepoint, Height height = height_v) const;
	Character get(Codepoint codepoint, Height height = height_v);

	std::size_t preload(std::span<Codepoint const> codepoints, Height height = height_v);

	std::string_view name() const { return m_name; }
	Ptr<Atlas const> atlas(Height height = height_v) const;
	Ptr<Texture const> texture(Height height = height_v) const;

  private:
	struct Face;
	struct Entry {
		Glyph glyph{};
		QuadTexCoords coords{};
	};
	struct Font {
		Atlas atlas{};
		ktl::hash_table<Codepoint, Entry> map{};
	};

	void on_loaded();
	Font& get_or_make(Height height);
	Entry& insert(Font& out_font, Codepoint codepoint, Atlas::Bulk* bulk);

	std::string m_name{};
	ktl::hash_table<Height, Font> m_fonts{};
	ktl::fixed_pimpl<Face, 64> m_face;
	std::unique_ptr<std::byte[]> m_file_data{};

	friend struct Scribe;
};
} // namespace vf
