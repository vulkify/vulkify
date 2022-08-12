#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/graphics/bitmap.hpp>
#include <vulkify/graphics/detail/resource.hpp>
#include <vulkify/graphics/texture_handle.hpp>

namespace vf {
class Context;

enum class AddressMode : std::uint8_t { eClampEdge, eClampBorder, eRepeat };
enum class Filtering : std::uint8_t { eNearest, eLinear };
enum class ImageFormat : std::uint8_t { eSrgb, eLinear };

struct TextureCreateInfo {
	AddressMode address_mode{AddressMode::eClampEdge};
	Filtering filtering{Filtering::eNearest};
	ImageFormat format{ImageFormat::eSrgb};
};

///
/// \brief Texture coordinates for a quad in texture space
///
struct QuadTexCoords {
	glm::ivec2 top_left{};
	glm::ivec2 bottom_right{};

	constexpr UvRect uv(glm::uvec2 const extent) const {
		auto const ext = glm::vec2(extent);
		return {glm::vec2(top_left) / ext, glm::vec2(bottom_right) / ext};
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
	explicit Texture(Context const& context, Image::View image = {}, CreateInfo const& create_info = {});

	Result<void> create(Image::View image);
	Result<void> overwrite(Image::View image, Rect const& region);
	Result<void> rescale(float scale);

	Texture clone() const;

	Extent extent() const;
	AddressMode address_mode() const { return m_address_mode; }
	Filtering filtering() const { return m_filtering; }
	UvRect uv(QuadTexCoords const coords) const { return coords.uv(extent()); }
	Handle handle() const;

  private:
	Texture(Vram const& vram, CreateInfo const& info);
	Texture clone_image() const;

	void refresh(Extent extent);
	void write(Image::View image, Rect const& region);
	void set_invalid();

	AddressMode m_address_mode{};
	Filtering m_filtering{};

	friend class Atlas;
};
} // namespace vf

#include <vulkify/graphics/detail/resource.hpp>
#include <vulkify/graphics/handle.hpp>

namespace vf::refactor {
class GfxImage;

///
/// \brief GPU Texture (Image and sampler)
///
class Texture : public GfxResource {
  public:
	using CreateInfo = TextureCreateInfo;
	using TopLeft = Bitmap::TopLeft;
	using Rect = TRect<std::uint32_t>;

	Texture() = default;
	explicit Texture(Context const& context, Image::View image = {}, CreateInfo const& create_info = {});

	Result<void> create(Image::View image);
	Result<void> overwrite(Image::View image, Rect const& region);
	Result<void> rescale(float scale);

	Texture clone() const;

	Extent extent() const;
	AddressMode address_mode() const { return m_address_mode; }
	Filtering filtering() const { return m_filtering; }
	UvRect uv(QuadTexCoords const coords) const { return coords.uv(extent()); }

	Handle<Texture> handle() const { return {m_allocation.get()}; }

  private:
	Texture(GfxDevice const* device, CreateInfo const& info);
	Texture clone_image(GfxImage& out_image) const;

	void refresh(GfxImage& out_image, Extent extent);
	void write(GfxImage& out_image, Image::View image, Rect const& region);
	void set_invalid(GfxImage& out_image);

	AddressMode m_address_mode{};
	Filtering m_filtering{};

	friend class Atlas;
};
} // namespace vf::refactor
