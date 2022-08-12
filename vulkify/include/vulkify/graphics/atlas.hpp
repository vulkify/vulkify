#pragma once
#include <vulkify/core/rect.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
///
/// \brief Expandable Texture Atlas
///
/// Store each quad's texture coords and obtain its updated UVs every frame via Atlas::uv()
///
class Atlas {
  public:
	static constexpr Extent initial_v = {1024, 128};
	static constexpr Rgba clear_v = Rgba{};
	class Bulk;

	Atlas() = default;
	explicit Atlas(Context const& context, Extent initial = initial_v, Rgba rgba = clear_v);

	///
	/// \brief Add image to atlas and obtain associated texture coordinates
	///
	/// Atlas expands texture height by default, unless width of image exceeds texture width,
	/// in whichh case it expands the width as well. Texture dimensions are always powers of two.
	///
	QuadTexCoords add(Image::View image);
	void clear(Rgba rgba = clear_v);

	Texture const& texture() const { return m_texture; }
	Extent extent() const { return texture().extent(); }
	UvRect uv(QuadTexCoords const coords) const { return coords.uv(extent()); }

  private:
	static constexpr glm::uvec2 pad_v = {1, 1};

	Atlas(Vram const& vram, Extent initial = initial_v, Rgba rgba = clear_v);

	void next_line();
	bool prepare(struct GfxCommandBuffer& cb, Extent extent);
	bool resize(GfxCommandBuffer& cb, Extent extent);
	bool overwrite(GfxCommandBuffer& cb, Image::View image, Texture::Rect const& region);
	QuadTexCoords insert(GfxCommandBuffer& cb, Image::View image);

	Texture m_texture{};

	struct {
		glm::uvec2 head{pad_v};
		std::uint32_t nextY{};
	} m_state{};

	friend struct GfxFont;
};

///
/// \brief Adds images to an Atlas in bulk
///
/// Note: There's no difference in the interface. Bulk::add simply collects all the underlying commands
/// and batch submits them to the GPU in its destructor. Atlas::add submits each image before returning.
///
class Atlas::Bulk {
  public:
	Bulk(Atlas& out_atlas);
	~Bulk();

	QuadTexCoords add(Image::View image);

  private:
	ktl::kunique_ptr<GfxCommandBuffer> m_impl;
	Atlas& m_atlas;
};
} // namespace vf

namespace vf::refactor {
struct GfxCommandBuffer;

///
/// \brief Expandable Texture Atlas
///
/// Store each quad's texture coords and obtain its updated UVs every frame via Atlas::uv()
///
class Atlas {
  public:
	static constexpr Extent initial_v = {1024, 128};
	static constexpr Rgba clear_v = Rgba{};
	class Bulk;

	Atlas() = default;
	explicit Atlas(Context const& context, Extent initial = initial_v, Rgba rgba = clear_v);

	///
	/// \brief Add image to atlas and obtain associated texture coordinates
	///
	/// Atlas expands texture height by default, unless width of image exceeds texture width,
	/// in whichh case it expands the width as well. Texture dimensions are always powers of two.
	///
	QuadTexCoords add(Image::View image);
	void clear(Rgba rgba = clear_v);

	Texture const& texture() const { return m_texture; }
	Extent extent() const { return texture().extent(); }
	UvRect uv(QuadTexCoords const coords) const { return coords.uv(extent()); }

  private:
	static constexpr glm::uvec2 pad_v = {1, 1};

	Atlas(GfxDevice const* device, Extent initial = initial_v, Rgba rgba = clear_v);

	void next_line();
	bool prepare(GfxCommandBuffer& cb, Extent extent);
	bool resize(GfxCommandBuffer& cb, Extent extent);
	bool overwrite(GfxCommandBuffer& cb, Image::View image, Texture::Rect const& region);
	QuadTexCoords insert(GfxCommandBuffer& cb, Image::View image);

	Texture m_texture{};

	struct {
		glm::uvec2 head{pad_v};
		std::uint32_t nextY{};
	} m_state{};

	friend class GfxFont;
};

///
/// \brief Adds images to an Atlas in bulk
///
/// Note: There's no difference in the interface. Bulk::add simply collects all the underlying commands
/// and batch submits them to the GPU in its destructor. Atlas::add submits each image before returning.
///
class Atlas::Bulk {
  public:
	Bulk(Atlas& out_atlas);
	~Bulk();

	QuadTexCoords add(Image::View image);

  private:
	ktl::kunique_ptr<GfxCommandBuffer> m_impl;
	Atlas& m_atlas;
};
} // namespace vf::refactor
