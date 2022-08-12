#pragma once
#include <vulkify/graphics/atlas.hpp>
#include <vulkify/ttf/character.hpp>
#include <vulkify/ttf/ttf_handle.hpp>
#include <memory>
#include <span>

namespace vf {
class Context;
struct GfxFont;

///
/// \brief TrueType Font
///
class Ttf {
  public:
	using Height = Glyph::Height;
	static constexpr auto height_v = Glyph::height_v;

	Ttf() noexcept;
	Ttf(Ttf&&) noexcept;
	Ttf& operator=(Ttf&&) noexcept;
	~Ttf() noexcept;

	explicit Ttf(Context const& context);

	explicit operator bool() const;

	bool load(std::span<std::byte const> bytes);
	bool load(char const* path);

	bool contains(Codepoint codepoint, Height height = height_v) const;
	Character find(Codepoint codepoint, Height height = height_v) const;
	Character get(Codepoint codepoint, Height height = height_v);

	std::size_t preload(std::span<Codepoint const> codepoints, Height height = height_v);

	Ptr<Atlas const> atlas(Height height = height_v) const;
	Ptr<Texture const> texture(Height height = height_v) const;
	TtfHandle handle() const { return {m_font.get()}; }

  private:
	void on_loaded();

	std::unique_ptr<std::byte[]> m_file_data{};
	std::unique_ptr<GfxFont> m_font{};
};
} // namespace vf

namespace vf::refactor {
///
/// \brief TrueType Font
///
class Ttf : public GfxResource {
  public:
	using Height = Glyph::Height;
	static constexpr auto height_v = Glyph::height_v;

	Ttf() = default;
	Ttf(Ttf&&) = default;
	Ttf& operator=(Ttf&&) = default;

	explicit Ttf(Context const& context);

	explicit operator bool() const;

	bool load(std::span<std::byte const> bytes);
	bool load(char const* path);

	bool contains(Codepoint codepoint, Height height = height_v) const;
	Character find(Codepoint codepoint, Height height = height_v) const;
	Character get(Codepoint codepoint, Height height = height_v);

	std::size_t preload(std::span<Codepoint const> codepoints, Height height = height_v);

	Ptr<Atlas const> atlas(Height height = height_v) const;
	Ptr<Texture const> texture(Height height = height_v) const;
	Handle<Ttf> handle() const { return m_handle; }

  private:
	void on_loaded(GfxFont& out_font);

	std::unique_ptr<std::byte[]> m_file_data{};
	Handle<Ttf> m_handle{};
};
} // namespace vf::refactor
