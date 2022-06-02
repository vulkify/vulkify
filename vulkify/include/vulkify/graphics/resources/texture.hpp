#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/graphics/bitmap.hpp>
#include <vulkify/graphics/resources/resource.hpp>
#include <vulkify/graphics/resources/texture_handle.hpp>

namespace vf {
class Context;

enum class AddressMode { eClampEdge, eClampBorder, eRepeat };
enum class Filtering { eNearest, eLinear };

struct TextureCreateInfo {
	AddressMode addressMode{AddressMode::eClampEdge};
	Filtering filtering{Filtering::eNearest};
};

///
/// \brief Texture coordinates for a quad in texture space
///
struct QuadTexCoords {
	glm::ivec2 topLeft{};
	glm::ivec2 bottomRight{};

	constexpr UvRect uv(glm::uvec2 const extent) const {
		auto const ext = glm::vec2(extent);
		return {glm::vec2(topLeft) / ext, glm::vec2(bottomRight) / ext};
	}
};

///
/// \brief GPU Texture (Image and sampler)
///
class Texture : public GfxResource {
  public:
	using CreateInfo = TextureCreateInfo;
	using TopLeft = Bitmap::TopLeft;
	using Rect = TRect<std::uint32_t>;
	using Handle = TextureHandle;

	Texture() = default;
	Texture(Context const& context, std::string name, Image::View image, CreateInfo const& createInfo = {});

	Result<void> create(Image::View image);
	Result<void> overwrite(Image::View image);
	Result<void> overwrite(Image::View image, Rect const& region);
	Result<void> rescale(float scale);

	Texture clone(std::string name) const;

	Extent extent() const;
	AddressMode addressMode() const { return m_addressMode; }
	Filtering filtering() const { return m_filtering; }
	UvRect uv(QuadTexCoords const coords) const { return coords.uv(extent()); }
	Handle handle() const;

  private:
	Texture(Vram const& vram, std::string name, CreateInfo const& info);
	Texture cloneImage(std::string name) const;

	void refresh(Extent extent);
	void write(Image::View image, Rect const& region);
	void setInvalid();

	AddressMode m_addressMode{};
	Filtering m_filtering{};

	friend class Atlas;
};
} // namespace vf
