#include <detail/trace.hpp>
#include <detail/vram.hpp>
#include <ttf/ft.hpp>
#include <vulkify/context/context.hpp>
#include <vulkify/ttf/ttf.hpp>
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

FtFace::Id FtFace::glyphIndex(std::uint32_t codepoint) const noexcept { return FT_Get_Char_Index(face, codepoint); }

bool FtFace::loadGlyph(Id index, FT_Render_Mode mode) const {
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

glm::uvec2 FtFace::glyphExtent() const {
	if (face && face->glyph) { return {face->glyph->bitmap.width, face->glyph->bitmap.rows}; }
	return {};
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

FtSlot FtFace::slot(std::uint32_t const codepoint) {
	auto ret = FtSlot{};
	FtFace::Id const id = codepoint == 0 ? 0 : glyphIndex(codepoint);
	if (loadGlyph(id)) {
		auto const& slot = *face->glyph;
		ret.metrics.advance = {slot.advance.x >> 6, slot.advance.y >> 6};
		ret.metrics.topLeft = {slot.bitmap_left, slot.bitmap_top};
		ret.pixmap.extent = {slot.bitmap.width, slot.bitmap.rows};
		ret.pixmap.bytes = buildGlyphImage();
		ret.pixmap.extent = glyphExtent();
	}
	return ret;
}

void FtDeleter::operator()(FtLib const& lib) const noexcept { FT_Done_FreeType(lib.lib); }
void FtDeleter::operator()(FtFace const& face) const noexcept { FT_Done_Face(face.face); }

// ttf

struct Ttf::Face {
	FtUnique<FtFace> face{};
	FtLib lib{};
};

static constexpr auto initial_extent_v = glm::uvec2(512, 128);

Ttf::Ttf() noexcept = default;
Ttf::Ttf(Ttf&&) noexcept = default;
Ttf& Ttf::operator=(Ttf&&) noexcept = default;
Ttf::~Ttf() noexcept = default;

Ttf::Ttf(Context const& context, std::string name) : m_atlas(context, name + "_atlas", initial_extent_v), m_name(std::move(name)) {
	if (!context.vram().ftlib) { return; }
	m_face->lib = *context.vram().ftlib;
}

Ttf::operator bool() const { return m_face->lib && m_face->face; }

bool Ttf::load(std::span<std::byte const> bytes) {
	if (!m_face->lib) { return false; }
	if (auto face = FtFace::make(m_face->lib, bytes)) {
		m_face->face = face;
		setHeight(64);
		return true;
	}
	return false;
}

bool Ttf::load(char const* path) {
	if (!m_face->lib) { return false; }
	if (auto face = FtFace::make(m_face->lib, path)) {
		m_face->face = face;
		setHeight(64);
		return true;
	}
	return false;
}

Ttf& Ttf::setHeight(std::uint32_t height) {
	if (m_height != height) {
		m_height = height;
		if (m_face->face) { m_face->face->setPixelSize({0, m_height}); }
		m_atlas.clear();
	}
	return *this;
}

Glyph const& Ttf::glyph(Codepoint codepoint) {
	static auto const blank_v = Glyph{};
	if (!*this) { return blank_v; }

	Entry* ret{};
	if (auto it = m_map.find(codepoint); it != m_map.end()) { ret = &it->second; }
	if (!ret) {
		auto entry = Entry{};
		auto const slot = m_face->face->slot(codepoint);
		if (slot.hasBitmap()) {
			auto const image = Image::View{slot.pixmap.bytes, slot.pixmap.extent};
			entry.id = m_atlas.add(image);
		}
		auto [it, _] = m_map.insert_or_assign(codepoint, std::move(entry));
		ret = &it->second;
	}

	if (ret) {
		ret->glyph.uv = m_atlas.get(ret->id);
		return ret->glyph;
	}
	return blank_v;
}
} // namespace vf
