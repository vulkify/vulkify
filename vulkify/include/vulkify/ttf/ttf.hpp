#pragma once
#include <vulkify/graphics/atlas.hpp>
#include <vulkify/ttf/character.hpp>
#include <memory>
#include <span>

namespace vf {
class GfxFont;

///
/// \brief TrueType Font
///
class Ttf : public GfxResource {
  public:
	using Height = Glyph::Height;
	static constexpr auto height_v = Glyph::height_v;

	Ttf() noexcept;
	Ttf(Ttf&&) noexcept;
	Ttf& operator=(Ttf&&) noexcept;
	~Ttf() noexcept;

	explicit Ttf(GfxDevice const& device);

	bool load(std::span<std::byte const> bytes);
	bool load(char const* path);

	bool contains(Codepoint codepoint, Height height = height_v) const;
	Character find(Codepoint codepoint, Height height = height_v) const;
	Character get(Codepoint codepoint, Height height = height_v);

	std::size_t preload(std::span<Codepoint const> codepoints, Height height = height_v);

	Ptr<Atlas const> atlas(Height height = height_v) const;
	Ptr<Texture const> texture(Height height = height_v) const;
	Handle<Ttf> handle() const;

  private:
	void on_loaded(GfxFont& out_font);

	std::unique_ptr<std::byte[]> m_file_data{};
	std::unique_ptr<GfxAllocation> m_allocation{};
};
} // namespace vf
