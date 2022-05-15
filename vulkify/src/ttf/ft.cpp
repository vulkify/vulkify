#include <detail/trace.hpp>
#include <detail/vram.hpp>
#include <ttf/ft.hpp>
#include <vulkify/context/context.hpp>
#include <vulkify/ttf/scribe.hpp>
#include <vulkify/ttf/ttf.hpp>
#include <exception>

namespace vf {
FtLib FtLib::make() noexcept {
	FtLib ret;
	if (FT_Init_FreeType(&ret.lib)) {
		VF_TRACE("[vf::Ttf] Failed to initialize freetype!");
		return {};
	}
	return ret;
}

FtFace FtFace::make(FT_Library lib, std::span<std::byte const> bytes) noexcept {
	static_assert(sizeof(FT_Byte) == sizeof(std::byte));
	FtFace ret;
	if (FT_New_Memory_Face(lib, reinterpret_cast<FT_Byte const*>(bytes.data()), static_cast<FT_Long>(bytes.size()), 0, &ret.face)) {
		VF_TRACE("[vf::Ttf] Failed to make font face");
		return {};
	}
	return ret;
}

FtFace FtFace::make(FT_Library lib, char const* path) noexcept {
	FtFace ret;
	if (FT_New_Face(lib, path, 0, &ret.face)) {
		VF_TRACE("[vf::Ttf] Failed to make font face");
		return {};
	}
	return ret;
}

bool FtFace::setCharSize(glm::uvec2 const size, glm::uvec2 const res) const noexcept {
	if (FT_Set_Char_Size(face, size.x, size.y, res.x, res.y)) {
		VF_TRACE("[vf::Ttf] Failed to set font face char size");
		return false;
	}
	return true;
}

bool FtFace::setPixelSize(glm::uvec2 const size) const noexcept {
	if (FT_Set_Pixel_Sizes(face, size.x, size.y)) {
		VF_TRACE("[vf::Ttf] Failed to set font face pixel size");
		return false;
	}
	return true;
}

FtFace::Id FtFace::glyphIndex(std::uint32_t codepoint) const noexcept { return FT_Get_Char_Index(face, codepoint); }

bool FtFace::loadGlyph(Id index, FT_Render_Mode mode) const {
	try {
		if (FT_Load_Glyph(face, index, FT_LOAD_DEFAULT)) {
			VF_TRACEF("[vf::Ttf] Failed to load glyph for index [{}]", index);
			return false;
		}
		if (FT_Render_Glyph(face->glyph, mode)) {
			VF_TRACEF("[vf::Ttf] Failed to render glyph for index [{}]", index);
			return false;
		}
	} catch (std::exception const& e) {
		VF_TRACEF("[vf::Ttf] Failed to load glyph for index [{}]: {}", index, e.what());
		return false;
	} catch (...) {
		VF_TRACEF("[vf::Ttf] Failed to load glyph for index [{}] (Unknown error)", index);
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

FtSlot FtFace::slot(std::uint32_t const codepoint) {
	auto ret = FtSlot{};
	FtFace::Id const id = codepoint == 0 ? 0 : glyphIndex(codepoint);
	if (loadGlyph(id)) {
		auto const& slot = *face->glyph;
		ret.metrics.advance = {slot.advance.x >> 6, slot.advance.y >> 6};
		ret.metrics.topLeft = {slot.bitmap_left, slot.bitmap_top};
		ret.metrics.extent = {slot.bitmap.width, slot.bitmap.rows};
		ret.pixmap = buildGlyphImage();
	}
	return ret;
}

void FtDeleter::operator()(FtLib const& lib) const noexcept { FT_Done_FreeType(lib.lib); }
void FtDeleter::operator()(FtFace const& face) const noexcept { FT_Done_Face(face.face); }

// ttf

struct Ttf::Face {
	FtUnique<FtFace> face{};
	FT_Library lib{};
};

static constexpr auto initial_extent_v = glm::uvec2(512, 128);

Ttf::Ttf() noexcept = default;
Ttf::Ttf(Ttf&&) noexcept = default;
Ttf& Ttf::operator=(Ttf&&) noexcept = default;
Ttf::~Ttf() noexcept = default;

Ttf::Ttf(Context const& context, std::string name) : m_atlas(context, name + "_atlas", initial_extent_v), m_name(std::move(name)) {
	if (!context.vram().ftlib) { return; }
	m_face->lib = context.vram().ftlib;
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
		glyph(0);
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
			auto const image = Image::View{slot.pixmap, slot.metrics.extent};
			entry.id = m_atlas.add(image);
		}
		entry.glyph.metrics = slot.metrics;
		auto [it, _] = m_map.insert_or_assign(codepoint, std::move(entry));
		ret = &it->second;
	}

	if (ret) {
		ret->glyph.uv = m_atlas.get(ret->id);
		return ret->glyph;
	}
	return blank_v;
}

std::size_t Ttf::preload(std::span<Codepoint const> codepoints) {
	auto ret = std::size_t{};
	auto bulk = Atlas::Bulk{m_atlas};
	for (auto const codepoint : codepoints) {
		if (m_map.contains(codepoint)) { continue; }
		Scribe::insert(bulk, *this, codepoint);
		++ret;
	}
	return ret;
}

Glyph const& Pen::glyph(Codepoint codepoint) const {
	static auto const blank_v = Glyph{};
	if (!out_ttf || !*out_ttf) { return blank_v; }
	if (auto const& glyph = out_ttf->glyph(codepoint)) { return glyph; }
	return out_ttf->glyph({});
}

glm::vec2 Pen::write(Codepoint const codepoint) {
	if (!out_ttf || !*out_ttf) { return head; }
	auto const& g = glyph(codepoint);
	if (g) {
		auto const pen = head + glm::vec2(g.metrics.topLeft);
		auto const hs = glm::vec2(g.metrics.extent) * 0.5f;
		auto const origin = pen + glm::vec2(hs.x, -hs.y);
		if (out_geometry) { out_geometry->addQuad({g.metrics.extent, origin, g.uv}); }
		head += g.metrics.advance;
		maxHeight = std::max(maxHeight, static_cast<float>(g.metrics.extent.y));
	}
	return head;
}

glm::vec2 Pen::write(std::span<Codepoint const> codepoints) {
	if (!out_ttf || !*out_ttf) { return head; }
	if (out_geometry) { out_geometry->reserve(codepoints.size() * 4, codepoints.size() * 6); }
	for (auto const codepoint : codepoints) { write(codepoint); }
	return head;
}

glm::vec2 Scribe::extent(std::string_view line) const {
	auto pen = Pen{&ttf};
	for (auto const ch : line) { pen.write(static_cast<Codepoint>(ch)); }
	return {pen.head.x, pen.maxHeight};
}

void Scribe::insert(Atlas::Bulk& out_bulk, Ttf& out_ttf, Codepoint codepoint) {
	if (out_ttf.m_map.contains(codepoint)) { return; }
	auto entry = Ttf::Entry{};
	auto const slot = out_ttf.m_face->face->slot(codepoint);
	if (slot.hasBitmap()) { entry.id = out_bulk.add({Image::View{slot.pixmap, slot.metrics.extent}}); }
	entry.glyph.metrics = slot.metrics;
	out_ttf.m_map.insert_or_assign(codepoint, std::move(entry));
}

Scribe& Scribe::preload(std::string_view text) {
	auto bulk = Atlas::Bulk(ttf.m_atlas);
	for (auto const ch : text) {
		auto const codepoint = static_cast<Codepoint>(ch);
		if (codepoint < codepoint_range_v.first || codepoint > codepoint_range_v.second || ttf.m_map.contains(codepoint)) { continue; }
		if (std::isspace(static_cast<unsigned char>(codepoint))) { continue; }
		insert(bulk, ttf, codepoint);
	}
	return *this;
}

Scribe& Scribe::write(std::string_view const text, Pivot pivot) {
	preload(text);
	auto start = origin;
	pivot += 0.5f;
	if (!FloatEq{}(pivot.x, 0.0f) || !FloatEq{}(pivot.y, 0.0f)) {
		auto const ext = extent(text);
		start -= ext * pivot;
	}
	geometry.reserve(text.size() * 4, text.size() * 6);
	auto pen = Pen{&ttf, &geometry, start};
	for (auto const ch : text) { pen.write(static_cast<Codepoint>(ch)); }
	return *this;
}

Scribe& Scribe::write(Block block, Pivot pivot) {
	if (!ttf || block.text.empty()) { return *this; }
	auto text = block.text;
	auto height = float{};
	auto const lh = lineHeight();
	for (auto line = std::string_view{}; block.getline(line);) { height += lh; }
	origin.y += height * (0.5f - pivot.y);
	block.text = text;
	for (auto line = std::string_view{}; block.getline(line); linebreak()) { write(line, {pivot.x, 0.5f}); }
	return *this;
}

float Scribe::lineHeight() const { return static_cast<float>(ttf.glyph(lineSpec.codepoint).metrics.extent.y) * lineSpec.coefficient; }

Scribe& Scribe::linebreak() {
	origin.y -= lineHeight();
	return *this;
}
} // namespace vf
