#pragma once
#include <vulkify/graphics/geometry.hpp>
#include <vulkify/ttf/ttf.hpp>

namespace vf {
///
/// \brief Splits lines in a loop
///
struct LineViewer {
	std::string_view text{};
	constexpr bool getline(std::string_view& out);
};

///
/// \brief Writes codepoints and moves write-head
///
struct Pen {
	GfxFont* out_font{};
	Geometry* out_geometry{};
	glm::vec2 head{};
	Glyph::Height height{Glyph::height_v};
	float max_height{};

	Character character(Codepoint codepoint) const;
	glm::vec2 write(Codepoint codepoint);
	glm::vec2 write(std::span<Codepoint const> codepoints);
};

///
/// \brief Writes aligned text using a Ttf
///
struct Scribe {
	using Height = Glyph::Height;
	using Pivot = glm::vec2;
	using Block = LineViewer;
	struct Leading {
		float coefficient{1.5f};
		Codepoint codepoint{'A'};
	};

	static constexpr auto centre_v = Pivot{};
	static constexpr auto codepoint_range_v = std::pair(Codepoint{33}, Codepoint{255});

	GfxFont& out_font;
	Height height{Glyph::height_v};
	glm::vec2 origin{};

	Geometry geometry{};
	Leading leading{};

	glm::vec2 extent(std::string_view line) const;
	float line_height() const;

	Scribe& preload(std::string_view text);
	Scribe& write(std::string_view text, Pivot pivot = centre_v);
	Scribe& write(Block text, Pivot pivot = centre_v);
	Scribe& line_break();
};

// impl

constexpr bool LineViewer::getline(std::string_view& out) {
	if (text.empty()) { return false; }
	auto const n = text.find('\n');
	out = text.substr(0, n);
	if (n != std::string_view::npos) {
		text = text.substr(n + 1);
	} else {
		text = {};
	}
	return true;
}
} // namespace vf
