#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <vulkify/core/rect.hpp>
#include <vulkify/core/unique.hpp>
#include <vulkify/ttf/glyph.hpp>
#include <span>

namespace vf {
struct FtLib {
	FT_Library lib{};

	static FtLib make() noexcept;

	explicit operator bool() const noexcept { return lib != nullptr; }
	constexpr bool operator==(FtLib const&) const = default;
};

struct FtSlot {
	Glyph::Metrics metrics{};
	std::vector<std::byte> pixmap{};

	bool hasBitmap() const noexcept { return !pixmap.empty(); }
};

struct FtFace {
	using Id = std::uint32_t;

	FT_Face face{};

	static FtFace make(FtLib const& lib, std::span<std::byte const> bytes) noexcept;
	static FtFace make(FtLib const& lib, char const* path) noexcept;

	explicit operator bool() const noexcept { return face != nullptr; }
	constexpr bool operator==(FtFace const&) const = default;

	bool setCharSize(glm::uvec2 size = {0U, 16U * 64U}, glm::uvec2 res = {300U, 300U}) const noexcept;
	bool setPixelSize(glm::uvec2 size = {0U, 16U}) const noexcept;

	Id glyphIndex(std::uint32_t codepoint) const noexcept;
	bool loadGlyph(Id index, FT_Render_Mode mode = FT_RENDER_MODE_NORMAL) const;
	std::vector<std::byte> buildGlyphImage() const;

	FtSlot slot(std::uint32_t codepoint);
};

struct FtDeleter {
	void operator()(FtLib const& lib) const noexcept;
	void operator()(FtFace const& face) const noexcept;
};

template <typename T, typename D = FtDeleter>
using FtUnique = Unique<T, D>;
} // namespace vf
