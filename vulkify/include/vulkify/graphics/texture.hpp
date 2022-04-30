#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/result.hpp>
#include <vulkify/graphics/bitmap.hpp>
#include <vulkify/graphics/resource.hpp>

namespace vf {
class Texture : public GfxResource {
  public:
	enum class Mode { eClampEdge, eClampBorder, eRepeat };
	enum class Filter { eNearest, eLinear };

	Texture() = default;
	Texture(Vram const& vram, std::string name, Bitmap bitmap, Mode mode = Mode::eClampEdge, Filter filter = Filter::eNearest);

	Result<void> create(Bitmap bitmap);
	Result<void> overwrite(Bitmap::View bitmap, Extent2D offset = {});

	Texture clone(std::string name) const;

	Extent2D extent() const;
	Bitmap const& bitmap() const { return m_bitmap; }
	Mode mode() const { return m_mode; }
	Filter filter() const { return m_filter; }

	void clearBitmap() { m_bitmap = {}; }

  private:
	void write(Bitmap::View bitmap, Extent2D offset);
	void setError();

	Bitmap m_bitmap{};
	Mode m_mode{};
	Filter m_filter{};
};
} // namespace vf
