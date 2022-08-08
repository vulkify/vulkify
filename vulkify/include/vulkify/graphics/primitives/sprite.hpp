#pragma once
#include <ktl/not_null.hpp>
#include <vulkify/core/defines.hpp>
#include <vulkify/graphics/geometry.hpp>
#include <vulkify/graphics/primitives/prop.hpp>

namespace vf {
class Texture;

///
/// \brief Primitive that models a textured quad
///
/// Supports sub texture UV indexing on sprite atlases
///
class Sprite : public Prop {
  public:
	class Sheet;
	///
	/// \brief Index into a sprite sheet's list of UVs
	///
	using UvIndex = std::uint32_t;

	Sprite() = default;
	explicit Sprite(Context const& context, glm::vec2 size = {100.0f, 100.0f});

	Sprite& set_size(glm::vec2 size);
	Sprite& set_sheet(Ptr<Sheet const> sheet, UvIndex index = {});

	glm::vec2 size() const { return m_state.size; }
	UvRect uv() const { return m_state.uv; }

	void draw(Surface const& surface, RenderState const& state) const override;

	///
	/// \brief If set, will draw a magenta-tinted quad if sprite doesn't have a sheet
	///
	bool draw_invalid = debug_v;

  protected:
	Sprite& set_uv_rect(UvRect uv);

	QuadCreateInfo m_state{};
};

///
/// \brief Stores a list of UVs and a TextureHandle
///
class Sprite::Sheet {
  public:
	TextureHandle texture() const { return m_texture; }

	///
	/// \brief Add a custom UV rect and obtain its corresponding UvIndex
	///
	UvIndex add_uv(UvRect uv);
	///
	/// \brief Obtain the UvRect corresponding to its UvIndex
	///
	/// Note: returns default UvRect if no indices present, first UvRect if passed index is out of bounds
	///
	UvRect const& uv(UvIndex index) const;
	std::size_t uv_count() const { return m_uvs.size(); }

	///
	/// \brief Auto-compute equi-sized UVs based on rows, columns, and padding between each cell
	///
	/// Note: All previously returned UvIndex values will be invalidated, new indices will be
	/// 	assigned in a monotonically increasing order, left to right, top to bottom: [0, uv_count()).
	///
	Sheet& set_uvs(ktl::not_null<Texture const*> texture, std::uint32_t rows = 1, std::uint32_t columns = 1, glm::uvec2 pad = {});
	Sheet& set_texture(TextureHandle texture);

  protected:
	std::vector<UvRect> m_uvs{};
	TextureHandle m_texture{};
};
} // namespace vf
