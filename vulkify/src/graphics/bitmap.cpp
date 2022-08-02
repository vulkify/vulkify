#include <vulkify/graphics/bitmap.hpp>
#include <iterator>

namespace vf {
namespace {
constexpr bool valid_offset(Extent view, Extent offset, Extent extent) { return view.x + offset.x < extent.x && view.y + offset.y < extent.y; }
} // namespace

Image Bitmap::View::image() const {
	auto bytes = std::make_unique<std::byte[]>(data.size_bytes() * Image::channels_v);
	auto head = bytes.get();
	for (auto const pixel : data) { head = rgba_to_byte(pixel, head); }
	auto ret = Image{};
	ret.replace({std::move(bytes), extent});
	return ret;
}

Bitmap::Bitmap(Rgba rgba, Extent const extent) {
	m_pixels.resize(extent.x * extent.y, rgba);
	m_extent = extent;
}

Bitmap::Bitmap(View bitmap) {
	if (!valid(bitmap)) { return; }
	m_pixels = {bitmap.data.begin(), bitmap.data.end()};
	m_extent = bitmap.extent;
}

bool Bitmap::overwrite(View view, TopLeft const offset) {
	if (!valid(view)) { return false; }
	if (!valid_offset(view.extent, offset, m_extent)) { return false; }
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
