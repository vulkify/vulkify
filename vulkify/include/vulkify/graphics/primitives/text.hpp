#pragma once
#include <ktl/not_null.hpp>
#include <vulkify/core/dirty_flag.hpp>
#include <vulkify/graphics/handle.hpp>
#include <vulkify/graphics/primitives/mesh.hpp>
#include <vulkify/ttf/glyph.hpp>

namespace vf {
class Ttf;

class Text : public Primitive {
  public:
	using Height = Glyph::Height;

	enum class Horz { eLeft, eCentre, eRight };
	enum class Vert { eDown, eMid, eUp };
	struct Align {
		Horz horz{Horz::eCentre};
		Vert vert{Vert::eMid};
	};

	Text() = default;
	explicit Text(GfxDevice const& device);

	explicit operator bool() const;

	Rgba& tint() { return m_mesh.t.storage.tint; }
	Rgba const& tint() const { return m_mesh.get().storage.tint; }
	Transform& transform() { return m_mesh.get().storage.transform; }
	Transform const& transform() const { return m_mesh.get().storage.transform; }
	std::string_view string() const { return m_text; }
	Align align() const { return m_align; }
	Height height() const { return m_height; }

	Text& set_ttf(ktl::not_null<Ttf*> ttf);
	Text& set_ttf(Handle<Ttf> ttf);
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
	Handle<Ttf> m_ttf{};
};
} // namespace vf
