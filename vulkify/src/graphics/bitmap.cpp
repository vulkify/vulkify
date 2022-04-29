#include <vulkify/graphics/bitmap.hpp>
#include <iterator>

namespace vf {
Bitmap::Bitmap(Rgba rgba, Extent2D const extent) {
	m_pixels.resize(extent.x * extent.y, rgba);
	m_extent = extent;
}

Bitmap::Bitmap(View bitmap) {
	if (!valid(bitmap)) { return; }
	m_pixels = {bitmap.pixels.begin(), bitmap.pixels.end()};
	m_extent = bitmap.extent;
}

bool Bitmap::overwrite(View view, glm::ivec2 const offset) {
	if (!valid(view)) { return false; }
	for (std::size_t row = 0; row < view.extent.x; ++row) {
		for (std::size_t col{}; col < view.extent.y; ++col) {
			int const r = static_cast<int>(row) + offset.x;
			int const c = static_cast<int>(col) + offset.y;
			if (r < 0 || c < 0) { continue; }
			auto const i2d = Index2D{static_cast<std::size_t>(r), static_cast<std::size_t>(c)};
			auto const index = i2d(m_extent.y);
			if (index >= m_pixels.size()) { continue; }
			m_pixels[index] = view[i2d];
		}
	}
	return true;
}
} // namespace vf
