#include <vulkify/graphics/bitmap.hpp>
#include <iterator>

namespace vf {
namespace {
constexpr bool validOffset(Extent2D view, Extent2D offset, Extent2D extent) { return view.x + offset.x < extent.x && view.y + offset.y < extent.y; }
} // namespace

Bitmap::Bitmap(Rgba rgba, Extent2D const extent) {
	m_pixels.resize(extent.x * extent.y, rgba);
	m_extent = extent;
}

Bitmap::Bitmap(View bitmap) {
	if (!valid(bitmap)) { return; }
	m_pixels = {bitmap.pixels.begin(), bitmap.pixels.end()};
	m_extent = bitmap.extent;
}

bool Bitmap::overwrite(View view, Extent2D const offset) {
	if (!valid(view)) { return false; }
	if (!validOffset(view.extent, offset, m_extent)) { return false; }
	for (std::uint32_t row = 0; row < view.extent.x; ++row) {
		for (std::uint32_t col{}; col < view.extent.y; ++col) {
			auto const r = row + offset.x;
			auto const c = col + offset.y;
			auto const i2d = Index2D{static_cast<std::size_t>(r), static_cast<std::size_t>(c)};
			auto const index = i2d(m_extent.y);
			assert(index < m_pixels.size());
			m_pixels[index] = view[i2d];
		}
	}
	return true;
}
} // namespace vf
