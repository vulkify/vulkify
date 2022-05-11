#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <glm/vec2.hpp>
#include <vulkify/core/unique.hpp>
#include <span>

namespace vf {
struct FtLib {
	FT_Library lib{};

	static FtLib make() noexcept;

	explicit operator bool() const noexcept { return lib != nullptr; }
	constexpr bool operator==(FtLib const&) const = default;
};

struct FtFace {
	using ID = std::uint32_t;

	FT_Face face{};

	static FtFace make(FtLib const& lib, std::span<std::byte const> bytes) noexcept;
	static FtFace make(FtLib const& lib, char const* path) noexcept;

	explicit operator bool() const noexcept { return face != nullptr; }
	constexpr bool operator==(FtFace const&) const = default;

	bool setCharSize(glm::uvec2 size = {0U, 16U * 64U}, glm::uvec2 res = {300U, 300U}) const noexcept;
	bool setPixelSize(glm::uvec2 size = {0U, 16U}) const noexcept;

	ID glyphIndex(std::uint32_t codepoint) const noexcept;
	bool loadGlyph(ID index, FT_Render_Mode mode = FT_RENDER_MODE_NORMAL) const;
	glm::uvec2 glyphExtent() const;
	std::vector<std::byte> buildGlyphImage() const;
};

struct FtPixmap {
	std::vector<std::byte> bytes{};
	glm::uvec2 extent{};
};

struct FtSlot {
	FtPixmap pixmap{};
	glm::ivec2 topLeft{};
	glm::ivec2 advance{};
	std::uint32_t codepoint{};

	bool hasBitmap() const noexcept { return pixmap.extent.x > 0 && pixmap.extent.y > 0; }

	static FtSlot make(FtFace face, std::uint32_t codepoint);
};

struct FtDeleter {
	void operator()(FtLib const& lib) const noexcept;
	void operator()(FtFace const& face) const noexcept;
};

template <typename T, typename D = FtDeleter>
using FtUnique = Unique<T, D>;
} // namespace vf
