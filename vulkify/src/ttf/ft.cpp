#include <detail/trace.hpp>
#include <ttf/ft.hpp>
#include <exception>

namespace vf {
FtLib FtLib::make() noexcept {
	FtLib ret;
	if (FT_Init_FreeType(&ret.lib)) {
		VF_TRACE("[vf::Font] Failed to initialize freetype!");
		return {};
	}
	return ret;
}

FtFace FtFace::make(FtLib const& lib, std::span<std::byte const> bytes) noexcept {
	static_assert(sizeof(FT_Byte) == sizeof(std::byte));
	FtFace ret;
	if (FT_New_Memory_Face(lib.lib, reinterpret_cast<FT_Byte const*>(bytes.data()), static_cast<FT_Long>(bytes.size()), 0, &ret.face)) {
		VF_TRACE("[vf::Font] Failed to make font face");
		return {};
	}
	return ret;
}

FtFace FtFace::make(FtLib const& lib, char const* path) noexcept {
	FtFace ret;
	if (FT_New_Face(lib.lib, path, 0, &ret.face)) {
		VF_TRACE("[vf::Font] Failed to make font face");
		return {};
	}
	return ret;
}

bool FtFace::setCharSize(glm::uvec2 const size, glm::uvec2 const res) const noexcept {
	if (FT_Set_Char_Size(face, size.x, size.y, res.x, res.y)) {
		VF_TRACE("[vf::Font] Failed to set font face char size");
		return false;
	}
	return true;
}

bool FtFace::setPixelSize(glm::uvec2 const size) const noexcept {
	if (FT_Set_Pixel_Sizes(face, size.x, size.y)) {
		VF_TRACE("[vf::Font] Failed to set font face pixel size");
		return false;
	}
	return true;
}

FtFace::ID FtFace::glyphIndex(std::uint32_t codepoint) const noexcept { return FT_Get_Char_Index(face, codepoint); }

bool FtFace::loadGlyph(ID index, FT_Render_Mode mode) const {
	try {
		if (FT_Load_Glyph(face, index, FT_LOAD_DEFAULT)) {
			VF_TRACEF("[vf::Font] Failed to load glyph for index [{}]", index);
			return false;
		}
		if (FT_Render_Glyph(face->glyph, mode)) {
			VF_TRACEF("[vf::Font] Failed to render glyph for index [{}]", index);
			return false;
		}
	} catch (std::exception const& e) {
		VF_TRACEF("[vf::Font] Failed to load glyph for index [{}]: {}", index, e.what());
		return false;
	} catch (...) {
		VF_TRACEF("[vf::Font] Failed to load glyph for index [{}] (Unknown error)", index);
		return false;
	}
	return true;
}

std::vector<std::byte> FtFace::buildGlyphImage() const {
	auto ret = std::vector<std::byte>{};
	if (face && face->glyph && face->glyph->bitmap.width > 0U && face->glyph->bitmap.rows > 0U) {
		glm::uvec2 const extent{face->glyph->bitmap.width, face->glyph->bitmap.rows};
		ret.reserve(extent.x & extent.y * 4U);
		auto line = reinterpret_cast<std::byte const*>(face->glyph->bitmap.buffer);
		static constexpr auto ff_v = static_cast<std::byte>(0xff);
		for (std::uint32_t row = 0; row < extent.y; ++row) {
			std::byte const* src = line;
			for (std::uint32_t col = 0; col < extent.x; ++col) {
				ret.push_back(ff_v);
				ret.push_back(ff_v);
				ret.push_back(ff_v);
				ret.push_back(*src++);
			}
			line += face->glyph->bitmap.pitch;
		}
	}
	return ret;
}

glm::uvec2 FtFace::glyphExtent() const {
	if (face && face->glyph) { return {face->glyph->bitmap.width, face->glyph->bitmap.rows}; }
	return {};
}

void FtDeleter::operator()(FtLib const& lib) const noexcept { FT_Done_FreeType(lib.lib); }
void FtDeleter::operator()(FtFace const& face) const noexcept { FT_Done_Face(face.face); }

FtSlot FtSlot::make(FtFace const face, std::uint32_t const cp) {
	FtSlot ret;
	FtFace::ID const id = cp == 0 ? 0 : face.glyphIndex(cp);
	if (face.loadGlyph(id)) {
		auto const& slot = *face.face->glyph;
		ret.codepoint = id == 0 ? 0 : cp;
		ret.advance = {slot.advance.x >> 6, slot.advance.y >> 6};
		ret.pixmap.extent = {slot.bitmap.width, slot.bitmap.rows};
		ret.pixmap.bytes = face.buildGlyphImage();
		ret.pixmap.extent = face.glyphExtent();
		ret.topLeft = {slot.bitmap_left, slot.bitmap_top};
	}
	return ret;
}
} // namespace vf
