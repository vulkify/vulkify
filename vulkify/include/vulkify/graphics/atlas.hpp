#pragma once
#include <ktl/hash_table.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/graphics/texture.hpp>

namespace vf {
class Atlas {
  public:
	static constexpr Extent initial_v = {1024, 128};
	using Id = std::uint32_t;

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
	bool prepare(Extent extent);
	bool resize(Extent extent);

	Texture m_texture{};
	ktl::hash_table<Id, UVRect> m_uvMap{};

	struct {
		glm::uvec2 head{pad_v};
		std::uint32_t nextY{};
		Id next{};
	} m_state{};
};
} // namespace vf
