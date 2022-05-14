#pragma once
#include <ktl/hash_table.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
struct GfxCommandBuffer;

///
/// \brief Expandable Texture Atlas
///
/// Note: since the Texture extent is dynamic, rect UVs may change on adding images.
/// Store each Id and query for its updated UV every frame if necessary - it's designed to be fast.
///
class Atlas {
  public:
	static constexpr Extent initial_v = {1024, 128};
	using Id = std::uint32_t;
	class Bulk;

	Atlas() = default;
	Atlas(Context const& context, std::string name, Extent initial = initial_v);

	///
	/// \brief Add image to atlas and obtain associated Id
	///
	/// Atlas expands texture height by default, unless width of image exceeds texture width,
	/// in whichh case it expands the width as well. Texture dimensions are always powers of two.
	///
	Id add(Image::View image);
	///
	/// \brief Obtain
	///
	UVRect get(Id id, UVRect const& fallback = {{}, {}}) const;

	bool contains(Id id) const { return m_uvMap.contains(id); }
	Extent extent() const { return m_texture.extent(); }
	std::size_t size() const { return m_uvMap.size(); }
	void clear();

	Texture const& texture() const { return m_texture; }

  private:
	static constexpr glm::uvec2 pad_v = {1, 1};

	void nextLine();
	bool prepare(GfxCommandBuffer& cb, Extent extent);
	bool resize(GfxCommandBuffer& cb, Extent extent);
	bool overwrite(GfxCommandBuffer& cb, Image::View image, Texture::Rect const& region);
	Id insert(GfxCommandBuffer& cb, Image::View image);

	Texture m_texture{};
	ktl::hash_table<Id, UVRect> m_uvMap{};

	struct {
		glm::uvec2 head{pad_v};
		std::uint32_t nextY{};
		Id next{};
	} m_state{};
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

	Id add(Image::View image);

  private:
	ktl::kunique_ptr<GfxCommandBuffer> m_impl;
	Atlas& m_atlas;
};
} // namespace vf
