#pragma once
#include <ktl/not_null.hpp>
#include <vulkify/core/dirty_flag.hpp>
#include <vulkify/graphics/handle.hpp>
#include <vulkify/graphics/primitives/mesh.hpp>
#include <vulkify/ttf/glyph.hpp>

namespace vf {
class Ttf;

class Text : public Primitive, public GfxResource {
  public:
	using Height = Glyph::Height;
	using Size = CharSize;

	enum class Horz { eLeft, eCentre, eRight };
	enum class Vert { eDown, eMid, eUp };
	struct Align {
		Horz horz{Horz::eCentre};
		Vert vert{Vert::eMid};
	};

	Text() = default;
	explicit Text(GfxDevice const& device);

	Rgba& tint() { return m_mesh.t.storage.tint; }
	Rgba const& tint() const { return m_mesh.get().storage.tint; }
	Transform& transform() { return m_mesh.get().storage.transform; }
	Transform const& transform() const { return m_mesh.get().storage.transform; }
	std::string_view string() const { return m_text; }
	Align align() const { return m_align; }
	Size size() const { return m_size; }

	Text& set_ttf(ktl::not_null<Ttf*> ttf);
	Text& set_ttf(Handle<Ttf> ttf);
	Text& set_string(std::string string);
	Text& append(std::string string);
	Text& append(char ch);
	Text& set_align(Align align);
	Text& set_height(Glyph::Height height);
	Text& set_scaling(float scaling);
	Text& set_size(Size size);

	void draw(Surface const& surface, RenderState const& state = {}) const override;

  private:
	void rebuild() const;

	DirtyFlag<Mesh> m_mesh{};
	std::string m_text{};
	Align m_align{};
	Size m_size{};
	Handle<Ttf> m_ttf{};
};
} // namespace vf
