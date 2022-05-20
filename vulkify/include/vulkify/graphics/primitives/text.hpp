#pragma once
#include <ktl/not_null.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/primitives/mesh.hpp>

namespace vf {
class Ttf;

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
	Text(Context const& context, std::string name);

	explicit operator bool() const;

	Text& setFont(ktl::not_null<Ttf*> ttf);

	Rgba& tint() { return m_mesh.instance.tint; }
	Rgba const& tint() const { return m_mesh.instance.tint; }
	Transform& transform() { return m_mesh.instance.transform; }
	Transform const& transform() const { return m_mesh.instance.transform; }

	void draw(Surface const& surface) const override;

	std::string text{};
	Align align{};
	Height height{60};

  protected:
	///
	/// \brief Requires m_ttf to be not-null
	///
	void update() const;

  private:
	mutable Mesh m_mesh{};
	Ttf* m_ttf{};
};
} // namespace vf
