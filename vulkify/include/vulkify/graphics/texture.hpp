#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/rect.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/graphics/bitmap.hpp>
#include <vulkify/graphics/resource.hpp>

namespace vf {
class Context;

enum class AddressMode { eClampEdge, eClampBorder, eRepeat };
enum class Filtering { eNearest, eLinear };

struct TextureCreateInfo {
	AddressMode addressMode{AddressMode::eClampEdge};
	Filtering filtering{Filtering::eNearest};
};

///
/// \brief GPU Texture (Image and sampler)
///
class Texture : public GfxResource {
  public:
	using CreateInfo = TextureCreateInfo;
	using TopLeft = Bitmap::TopLeft;
	using Rect = TRect<std::uint32_t>;

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
