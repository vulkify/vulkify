#pragma once
#include <vulkify/graphics/geometry.hpp>
#include <vulkify/ttf/ttf.hpp>

namespace vf {
struct Pen {
	Ttf* out_ttf{};
	Geometry* out_geometry{};
	glm::vec2 head{};
	float maxHeight{};

	glm::vec2 write(Codepoint const codepoint);
	glm::vec2 write(std::span<Codepoint const> codepoints);
};

struct LineViewer {
	std::string_view text{};
	constexpr bool getline(std::string_view& out);
};

struct Scribe {
	using Pivot = glm::vec2;
	using Block = LineViewer;
	struct LineSpec {
		float coefficient{1.5f};
		Codepoint codepoint{'A'};
	};

	static constexpr auto centre_v = Pivot{};

	Ttf& ttf;
	glm::vec2 origin{};

	LineSpec lineSpec{};
	Geometry geometry{};

	glm::vec2 extent(std::string_view line) const;
	float lineHeight() const;

	Scribe& write(std::string_view text, Pivot pivot = centre_v);
	Scribe& write(Block text, Pivot pivot = centre_v);
	Scribe& linebreak();
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