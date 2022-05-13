#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <ktl/hash_table.hpp>
#include <vulkify/graphics/atlas.hpp>
#include <vulkify/ttf/glyph.hpp>

namespace vf {
class Ttf {
  public:
	Ttf() noexcept;
	Ttf(Ttf&&) noexcept;
	Ttf& operator=(Ttf&&) noexcept;
	~Ttf() noexcept;

	Ttf(Context const& context, std::string name);
	explicit operator bool() const;

	bool load(std::span<std::byte const> bytes);
	bool load(char const* path);

	std::uint32_t height() const { return m_height; }
	Ttf& setHeight(std::uint32_t size);

	bool contains(Codepoint codepoint) const { return m_map.contains(codepoint); }
	Glyph const& glyph(Codepoint codepoint);

	std::string_view name() const { return m_name; }
	Atlas const& atlas() const { return m_atlas; }
	Texture const& texture() const { return m_atlas.texture(); }

  private:
	struct Face;
	struct Entry {
		Glyph glyph{};
		Atlas::Id id{};
	};

	Atlas m_atlas{};
	std::string m_name{};
	ktl::hash_table<std::uint32_t, Entry> m_map{};
	ktl::fixed_pimpl<Face, 32> m_face;
	std::uint32_t m_height{};
};
} // namespace vf
