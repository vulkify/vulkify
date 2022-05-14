#pragma once
#include <ktl/hash_table.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
struct GfxCommandBuffer;

class Atlas {
  public:
	static constexpr Extent initial_v = {1024, 128};
	using Id = std::uint32_t;
	class Bulk;

	Atlas() = default;
	Atlas(Context const& context, std::string name, Extent initial = initial_v);

	Id add(Image::View image);
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
