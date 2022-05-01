#pragma once
#include <vulkify/core/rgba.hpp>
#include <vulkify/graphics/image.hpp>
#include <limits>
#include <utility>
#include <vector>

namespace vf {
using Extent2D = glm::uvec2;

struct Index2D {
	std::size_t row{};
	std::size_t col{};

	static constexpr Index2D make(std::size_t index, std::size_t cols) { return {index / cols, index % cols}; }
	constexpr std::size_t operator()(std::size_t rows) const { return row * rows + col; }
};

class Bitmap {
  public:
	struct View {
		std::span<Rgba const> pixels{};
		Extent2D extent{1, 1};

		constexpr Rgba operator[](Index2D index) const { return pixels[index(extent.y)]; }
		Image image() const;
	};

	template <std::output_iterator<std::byte> Out>
	static constexpr Out rgbaToByte(Rgba const rgba, Out out);

	Bitmap() = default;

	explicit Bitmap(Rgba rgba, Extent2D extent = {1, 1});
	explicit Bitmap(View bitmap);

	static constexpr bool valid(Bitmap::View const& bitmap);

	operator View() const { return {m_pixels, m_extent}; }
	explicit operator bool() const { return valid(*this); }

	Extent2D extent() const { return m_extent; }
	std::span<Rgba> pixels() { return m_pixels; }
	std::span<Rgba const> pixels() const { return m_pixels; }
	Rgba& operator[](Index2D index) { return m_pixels.at(index(m_extent.y)); }
	Rgba const& operator[](Index2D index) const { return m_pixels.at(index(m_extent.y)); }

	bool overwrite(View view, Extent2D offset = {});
	Image image() const { return static_cast<View>(*this).image(); }

  private:
	std::vector<Rgba> m_pixels{};
	Extent2D m_extent{};
};

// impl

constexpr bool Bitmap::valid(Bitmap::View const& bitmap) {
	return bitmap.extent.x > 0 && bitmap.extent.y > 0 && Image::sizeBytes(bitmap.extent) == bitmap.pixels.size_bytes();
}

template <std::output_iterator<std::byte> Out>
constexpr Out Bitmap::rgbaToByte(Rgba const rgba, Out out) {
	auto add = [&out](Rgba::Channel const channel) { *out++ = static_cast<std::byte>(channel); };
	add(rgba.channels[0]);
	add(rgba.channels[1]);
	add(rgba.channels[2]);
	add(rgba.channels[3]);
	return out;
}
} // namespace vf