#pragma once
#include <glm/vec2.hpp>
#include <vulkify/core/rgba.hpp>
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
	};

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
	std::size_t byteSize() const { return m_extent.x * m_extent.y * 4; }

  private:
	std::vector<Rgba> m_pixels{};
	Extent2D m_extent{};
};

// impl

constexpr bool Bitmap::valid(Bitmap::View const& bitmap) {
	return bitmap.extent.x > 0 && bitmap.extent.y > 0 && bitmap.pixels.size() == bitmap.extent.x * bitmap.extent.y;
}
} // namespace vf
