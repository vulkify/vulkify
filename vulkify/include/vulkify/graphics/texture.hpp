#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/graphics/bitmap.hpp>
#include <vulkify/graphics/resource.hpp>

namespace vf {
enum class AddressMode { eClampEdge, eClampBorder, eRepeat };
enum class Filtering { eNearest, eLinear };

struct TextureCreateInfo {
	AddressMode addressMode{AddressMode::eClampEdge};
	Filtering filtering{Filtering::eNearest};
};

class Texture : public GfxResource {
  public:
	using CreateInfo = TextureCreateInfo;

	Texture() = default;
	Texture(Vram const& vram, std::string name, Bitmap bitmap, CreateInfo const& createInfo = {});

	Result<void> create(Bitmap bitmap);
	Result<void> overwrite(Bitmap::View bitmap, Extent2D offset = {});

	Texture clone(std::string name) const;

	Extent2D extent() const;
	Bitmap const& bitmap() const { return m_bitmap; }
	AddressMode addressMode() const { return m_addressMode; }
	Filtering filtering() const { return m_filtering; }

	void clearBitmap() { m_bitmap = {}; }

  private:
	void write(Bitmap::View bitmap, Extent2D offset);
	void setInvalid();

	Bitmap m_bitmap{};
	AddressMode m_addressMode{};
	Filtering m_filtering{};
};
} // namespace vf
