#include <detail/gfx_font.hpp>
#include <detail/trace.hpp>
#include <detail/vram.hpp>
#include <ttf/ft.hpp>
#include <vulkify/context/context.hpp>
#include <vulkify/ttf/scribe.hpp>
#include <vulkify/ttf/ttf.hpp>
#include <exception>
#include <sstream>

namespace vf {
[[maybe_unused]] static constexpr auto name_v = "vf::Ttf";

namespace {
constexpr int get_space(Codepoint const cp) {
	switch (static_cast<char>(cp)) {
	case ' ': return 1;
	case '\t': return 4;
	default: return 0;
	}
}
} // namespace

FtLib FtLib::make() noexcept {
	FtLib ret;
	if (FT_Init_FreeType(&ret.lib)) {
		VF_TRACE(name_v, trace::Type::eError, "Failed to initialize freetype!");
		return {};
	}
	return ret;
}

FtFace FtFace::make(FT_Library lib, std::span<std::byte const> bytes) noexcept {
	static_assert(sizeof(FT_Byte) == sizeof(std::byte));
	FtFace ret;
	if (FT_New_Memory_Face(lib, reinterpret_cast<FT_Byte const*>(bytes.data()), static_cast<FT_Long>(bytes.size()), 0, &ret.face)) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to make font face");
		return {};
	}
	return ret;
}

FtFace FtFace::make(FT_Library lib, char const* path) noexcept {
	FtFace ret;
	if (FT_New_Face(lib, path, 0, &ret.face)) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to make font face");
		return {};
	}
	return ret;
}

bool FtFace::set_char_size(glm::uvec2 const size, glm::uvec2 const res) const noexcept {
	if (FT_Set_Char_Size(face, size.x, size.y, res.x, res.y)) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to set font face char size");
		return false;
	}
	return true;
}

bool FtFace::set_pixel_size(glm::uvec2 const size) const noexcept {
	if (FT_Set_Pixel_Sizes(face, size.x, size.y)) {
		VF_TRACE(name_v, trace::Type::eWarn, "Failed to set font face pixel size");
		return false;
	}
	return true;
}

FtFace::Id FtFace::glyph_index(std::uint32_t codepoint) const noexcept { return FT_Get_Char_Index(face, codepoint); }

bool FtFace::load_glyph(Id index, FT_Render_Mode mode) const {
	try {
		if (FT_Load_Glyph(face, index, FT_LOAD_DEFAULT)) {
			VF_TRACEW(name_v, "Failed to load glyph for index [{}]", index);
			return false;
		}
		if (FT_Render_Glyph(face->glyph, mode)) {
			VF_TRACEW(name_v, "Failed to render glyph for index [{}]", index);
			return false;
		}
	} catch (std::exception const& e) {
		VF_TRACEW(name_v, "Failed to load glyph for index [{}]: {}", index, e.what());
		return false;
	} catch (...) {
		VF_TRACEW(name_v, "Failed to load glyph for index [{}] (Unknown error)", index);
		return false;
	}
	return true;
}

std::vector<std::byte> FtFace::build_glyph_image() const {
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
	FtFace::Id const id = codepoint == 0 ? 0 : glyph_index(codepoint);
	if (load_glyph(id)) {
		auto const& slot = *face->glyph;
		ret.metrics.advance = {slot.advance.x >> 6, slot.advance.y >> 6};
		ret.metrics.topLeft = {slot.bitmap_left, slot.bitmap_top};
		ret.metrics.extent = {slot.bitmap.width, slot.bitmap.rows};
		ret.pixmap = build_glyph_image();
	}
	return ret;
}

void FtDeleter::operator()(FtLib const& lib) const noexcept { FT_Done_FreeType(lib.lib); }
void FtDeleter::operator()(FtFace const& face) const noexcept { FT_Done_Face(face.face); }

// ttf

static constexpr auto initial_extent_v = glm::uvec2(512, 128);

GfxFont::GfxFont(Context const& context) {
	if (!context.vram().ftlib) { return; }
	face.vram = &context.vram();
}

GfxFont::operator bool() const { return face.vram && face.face; }

Character GfxFont::get(Codepoint codepoint, Height height) {
	if (!*this) { return {}; }
	auto& font = get_or_make(height);

	Entry const* ret{};
	if (auto it = font.map.find(codepoint); it != font.map.end()) { ret = &it->second; }
	if (!ret) { ret = &insert(font, codepoint, {}); }

	if (ret) { return {&ret->glyph, font.atlas.uv(ret->coords)}; }
	return {};
}

GfxFont::Font& GfxFont::get_or_make(Height height) {
	auto it = fonts.find(height);
	if (it == fonts.end()) {
		auto [i, _] = fonts.insert_or_assign(height, Font{Atlas(*face.vram, initial_extent_v)});
		it = i;
		insert(it->second, {}, nullptr);
	}
	face.face->set_pixel_size({0, height});
	return it->second;
}

GfxFont::Entry& GfxFont::insert(Font& out_font, Codepoint const codepoint, Atlas::Bulk* bulk) {
	auto const slot = face.face->slot(codepoint);
	auto entry = Entry{};
	if (slot.has_bitmap()) {
		auto const image = Image::View{slot.pixmap, slot.metrics.extent};
		if (bulk) {
			entry.coords = bulk->add(image);
		} else {
			entry.coords = out_font.atlas.add(image);
		}
	}
	entry.glyph.metrics = slot.metrics;
	auto [it, _] = out_font.map.insert_or_assign(codepoint, std::move(entry));
	return it->second;
}

Ptr<Atlas const> GfxFont::atlas(Height height) const {
	if (auto it = fonts.find(height); it != fonts.end()) { return &it->second.atlas; }
	return {};
}

Ptr<Texture const> GfxFont::texture(Height height) const {
	if (auto a = atlas(height)) { return &a->texture(); }
	return {};
}

Ttf::Ttf() noexcept = default;
Ttf::Ttf(Ttf&&) noexcept = default;
Ttf& Ttf::operator=(Ttf&&) noexcept = default;
Ttf::~Ttf() noexcept = default;

Ttf::Ttf(Context const& context) : m_font(std::make_unique<GfxFont>(context)) {}

Ttf::operator bool() const { return m_font && *m_font; }

bool Ttf::load(std::span<std::byte const> bytes) {
	if (!m_font || !m_font->face.vram) { return false; }
	auto data = std::make_unique<std::byte[]>(bytes.size());
	std::memcpy(data.get(), bytes.data(), bytes.size());
	if (auto face = FtFace::make(m_font->face.vram->ftlib, {data.get(), bytes.size()})) {
		m_font->face.face = face;
		m_file_data = std::move(data);
		on_loaded();
		return true;
	}
	return false;
}

bool Ttf::load(char const* path) {
	if (!m_font || !m_font->face.vram) { return false; }
	if (auto face = FtFace::make(m_font->face.vram->ftlib, path)) {
		m_font->face.face = face;
		on_loaded();
		return true;
	}
	return false;
}

bool Ttf::contains(Codepoint codepoint, Height height) const {
	if (!m_font) { return false; }
	if (auto it = m_font->fonts.find(height); it != m_font->fonts.end()) { return it->second.map.contains(codepoint); }
	return false;
}

Character Ttf::find(Codepoint codepoint, Height height) const {
	if (!*this) { return {}; }
	auto font_it = m_font->fonts.find(height);
	if (font_it == m_font->fonts.end()) { return {}; }

	auto& font = font_it->second;
	auto entry_it = font.map.find(codepoint);
	if (entry_it == font.map.end()) { return {}; }

	auto& entry = entry_it->second;
	return {&entry.glyph, font.atlas.uv(entry.coords)};
}

Character Ttf::get(Codepoint codepoint, Height height) {
	if (!*this) { return {}; }
	return m_font->get(codepoint, height);
}

std::size_t Ttf::preload(std::span<Codepoint const> codepoints, Height const height) {
	if (!m_font || !m_font->face.face) { return {}; }
	auto ret = std::size_t{};
	auto& font = m_font->get_or_make(height);
	auto bulk = Atlas::Bulk{font.atlas};
	for (auto const codepoint : codepoints) {
		if (font.map.contains(codepoint)) { continue; }
		m_font->insert(font, codepoint, &bulk);
		++ret;
	}
	return ret;
}

Ptr<Atlas const> Ttf::atlas(Height height) const { return m_font ? m_font->atlas(height) : nullptr; }

Ptr<Texture const> Ttf::texture(Height height) const { return m_font ? m_font->texture(height) : nullptr; }

void Ttf::on_loaded() {
	assert(m_font->face.vram);
	m_font->fonts.clear();
	m_font->get_or_make(height_v);
}

Character Pen::character(Codepoint codepoint) const {
	if (!out_font || !*out_font) { return {}; }
	if (auto const ch = out_font->get(codepoint, height)) { return ch; }
	return out_font->get({}, height);
}

glm::vec2 Pen::write(Codepoint const codepoint) {
	if (!out_font || !*out_font) { return head; }
	if (auto space = get_space(codepoint); space > 0) {
		head += space * character('i').glyph->metrics.advance;
		return head;
	}
	if (auto const ch = character(codepoint)) {
		auto const pen = head + glm::vec2(ch.glyph->metrics.topLeft);
		auto const hs = glm::vec2(ch.glyph->metrics.extent) * 0.5f;
		auto const origin = pen + glm::vec2(hs.x, -hs.y);
		if (out_geometry) { out_geometry->add_quad({ch.glyph->metrics.extent, origin, ch.uv}); }
		head += ch.glyph->metrics.advance;
		max_height = std::max(max_height, static_cast<float>(ch.glyph->metrics.extent.y));
	}
	return head;
}

glm::vec2 Pen::write(std::span<Codepoint const> codepoints) {
	if (!out_font || !*out_font) { return head; }
	if (out_geometry) { out_geometry->reserve(codepoints.size() * 4, codepoints.size() * 6); }
	for (auto const codepoint : codepoints) { write(codepoint); }
	return head;
}

glm::vec2 Scribe::extent(std::string_view line) const {
	auto pen = Pen{&out_font, {}, {}, height};
	for (auto const ch : line) { pen.write(static_cast<Codepoint>(ch)); }
	return {pen.head.x, pen.max_height};
}

Scribe& Scribe::preload(std::string_view text) {
	auto& font = out_font.get_or_make(height);
	auto bulk = Atlas::Bulk(font.atlas);
	for (auto const ch : text) {
		auto const codepoint = static_cast<Codepoint>(ch);
		if (codepoint < codepoint_range_v.first || codepoint > codepoint_range_v.second || font.map.contains(codepoint)) { continue; }
		if (std::isspace(static_cast<unsigned char>(codepoint))) { continue; }
		out_font.insert(font, codepoint, &bulk);
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
	auto pen = Pen{&out_font, &geometry, start, height};
	for (auto const ch : text) { pen.write(static_cast<Codepoint>(ch)); }
	return *this;
}

Scribe& Scribe::write(Block block, Pivot pivot) {
	if (!out_font || block.text.empty()) { return *this; }
	auto text = block.text;
	auto max_line_height = 0.0f;
	auto line_count = 0;
	for (auto line = std::string_view{}; block.getline(line);) {
		max_line_height = std::max(max_line_height, extent(line).y);
		++line_count;
	}
	auto const dy = max_line_height * leading.coefficient;
	auto const height = line_count == 1 ? max_line_height : dy * static_cast<float>(line_count);
	origin.y += height * (0.5f - pivot.y);
	block.text = text;
	for (auto line = std::string_view{}; block.getline(line); origin.y -= dy) { write(line, {pivot.x, 0.5f}); }
	return *this;
}

float Scribe::line_height() const {
	auto const ch = out_font.get(leading.codepoint, height);
	if (!ch) { return {}; }
	return static_cast<float>(ch.glyph->metrics.extent.y) * leading.coefficient;
}

Scribe& Scribe::line_break() {
	origin.y -= line_height();
	return *this;
}
} // namespace vf

#include <detail/gfx_device.hpp>

namespace vf::refactor {
GfxFont::GfxFont(GfxDevice const* device) : GfxAllocation(device, GfxAllocation::Type::eFont) {
	if (!device->ftlib) { return; }
}

GfxFont::operator bool() const { return device() && face->face; }

Character GfxFont::get(Codepoint codepoint, Height height) {
	if (!*this) { return {}; }
	auto& font = get_or_make(height);

	Entry const* ret{};
	if (auto it = font.map.find(codepoint); it != font.map.end()) { ret = &it->second; }
	if (!ret) { ret = &insert(font, codepoint, {}); }

	if (ret) { return {&ret->glyph, font.atlas.uv(ret->coords)}; }
	return {};
}

GfxFont::Font& GfxFont::get_or_make(Height height) {
	auto it = fonts.find(height);
	if (it == fonts.end()) {
		auto [i, _] = fonts.insert_or_assign(height, Font{Atlas{device(), initial_extent_v}});
		it = i;
		insert(it->second, {}, nullptr);
	}
	face->set_pixel_size({0, height});
	return it->second;
}

GfxFont::Entry& GfxFont::insert(Font& out_font, Codepoint const codepoint, Atlas::Bulk* bulk) {
	auto const slot = face->slot(codepoint);
	auto entry = Entry{};
	if (slot.has_bitmap()) {
		auto const image = Image::View{slot.pixmap, slot.metrics.extent};
		if (bulk) {
			entry.coords = bulk->add(image);
		} else {
			entry.coords = out_font.atlas.add(image);
		}
	}
	entry.glyph.metrics = slot.metrics;
	auto [it, _] = out_font.map.insert_or_assign(codepoint, std::move(entry));
	return it->second;
}

Ptr<Atlas const> GfxFont::atlas(Height height) const {
	if (auto it = fonts.find(height); it != fonts.end()) { return &it->second.atlas; }
	return {};
}

Ptr<Texture const> GfxFont::texture(Height height) const {
	if (auto a = atlas(height)) { return &a->texture(); }
	return {};
}

Ttf::Ttf(Context const& context) : GfxResource(&context.device()) {
	if (!m_device) { return; }
	auto font = ktl::make_unique<GfxFont>(m_device);
	auto lock = std::scoped_lock(m_device->allocations->mutex);
	m_handle = {m_device->allocations->add(lock, std::move(font)).value};
}

Ttf::operator bool() const { return m_device && m_handle; }

bool Ttf::load(std::span<std::byte const> bytes) {
	auto* font = m_device ? m_device->as<GfxFont>(m_allocation) : nullptr;
	if (!font || !font->device()->ftlib) { return false; }
	auto data = std::make_unique<std::byte[]>(bytes.size());
	std::memcpy(data.get(), bytes.data(), bytes.size());
	if (auto face = FtFace::make(font->device()->ftlib, {data.get(), bytes.size()})) {
		font->face = face;
		m_file_data = std::move(data);
		on_loaded(*font);
		return true;
	}
	return false;
}

bool Ttf::load(char const* path) {
	auto* font = m_device ? m_device->as<GfxFont>(m_allocation) : nullptr;
	if (!font) { return false; }
	if (auto face = FtFace::make(m_device->ftlib, path)) {
		font->face = face;
		on_loaded(*font);
		return true;
	}
	return false;
}

bool Ttf::contains(Codepoint codepoint, Height height) const {
	auto* font = m_device ? m_device->as<GfxFont>(m_allocation) : nullptr;
	if (!font) { return false; }
	if (auto it = font->fonts.find(height); it != font->fonts.end()) { return it->second.map.contains(codepoint); }
	return false;
}

Character Ttf::find(Codepoint codepoint, Height height) const {
	auto* gfx_font = m_device ? m_device->as<GfxFont>(m_allocation) : nullptr;
	if (!gfx_font) { return {}; }
	auto font_it = gfx_font->fonts.find(height);
	if (font_it == gfx_font->fonts.end()) { return {}; }

	auto& font = font_it->second;
	auto entry_it = font.map.find(codepoint);
	if (entry_it == font.map.end()) { return {}; }

	auto& entry = entry_it->second;
	return {&entry.glyph, font.atlas.uv(entry.coords)};
}

Character Ttf::get(Codepoint codepoint, Height height) {
	auto* font = m_device ? m_device->as<GfxFont>(m_allocation) : nullptr;
	if (!font) { return {}; }
	return font->get(codepoint, height);
}

std::size_t Ttf::preload(std::span<Codepoint const> codepoints, Height const height) {
	auto* gfx_font = m_device ? m_device->as<GfxFont>(m_allocation) : nullptr;
	if (!gfx_font) { return {}; }
	auto ret = std::size_t{};
	auto& font = gfx_font->get_or_make(height);
	auto bulk = Atlas::Bulk{font.atlas};
	for (auto const codepoint : codepoints) {
		if (font.map.contains(codepoint)) { continue; }
		gfx_font->insert(font, codepoint, &bulk);
		++ret;
	}
	return ret;
}

Ptr<Atlas const> Ttf::atlas(Height height) const {
	auto* font = m_device ? m_device->as<GfxFont>(m_allocation) : nullptr;
	return font ? font->atlas(height) : nullptr;
}

Ptr<Texture const> Ttf::texture(Height height) const {
	auto* font = m_device ? m_device->as<GfxFont>(m_allocation) : nullptr;
	return font ? font->texture(height) : nullptr;
}

void Ttf::on_loaded(GfxFont& out_font) {
	assert(out_font.device());
	out_font.fonts.clear();
	out_font.get_or_make(height_v);
}
} // namespace vf::refactor
