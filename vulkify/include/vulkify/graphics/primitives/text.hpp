#pragma once
#include <ktl/not_null.hpp>
#include <vulkify/core/dirty_flag.hpp>
#include <vulkify/graphics/primitives/mesh.hpp>
#include <vulkify/graphics/resources/geometry_buffer.hpp>

namespace vf {
class Ttf;
class GfxFont;

class Text : public Primitive {
  public:
	using Height = std::uint32_t;

	enum class Horz { eLeft, eCentre, eRight };
	enum class Vert { eDown, eMid, eUp };
	struct Align {
		Horz horz{Horz::eCentre};
		Vert vert{Vert::eMid};
	};

	Text() = default;
	explicit Text(Context const& context);

	explicit operator bool() const;

	Rgba& tint() { return m_mesh.t.storage.tint; }
	Rgba const& tint() const { return m_mesh.get().storage.tint; }
	Transform& transform() { return m_mesh.get().storage.transform; }
	Transform const& transform() const { return m_mesh.get().storage.transform; }
	std::string const& string() const& { return m_text; }
	Align align() const { return m_align; }
	Height height() const { return m_height; }

	Text& set_font(ktl::not_null<Ttf*> ttf);
	Text& set_font(ktl::not_null<GfxFont*> font);
	Text& set_string(std::string string);
	Text& append(std::string string);
	Text& append(char ch);
	Text& set_align(Align align);
	Text& set_height(Height height);

	void draw(Surface const& surface, RenderState const& state = {}) const override;

  private:
	void rebuild() const;

	DirtyFlag<Mesh> m_mesh{};
	std::string m_text{};
	Align m_align{};
	Height m_height{60};
	Ptr<GfxFont> m_font{};
};
} // namespace vf
