#include <stb/stb_image.h>
#include <ktl/either.hpp>
#include <vulkify/core/unique.hpp>
#include <vulkify/graphics/image.hpp>
#include <optional>

namespace vf {
namespace {
struct StbiDeleter {
	void operator()(stbi_uc* data) const { stbi_image_free(data); }
};

static constexpr auto stbi_ok = 1;

using Stbi = std::unique_ptr<stbi_uc, StbiDeleter>;
using Bytes = std::unique_ptr<std::byte[]>;

constexpr Extent2D makeExtent(int x, int y) { return {static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y)}; }

Stbi loadImage(Image::Encoded encoded, Extent2D& out_extent) {
	int x, y, ch;
	auto const data = reinterpret_cast<stbi_uc const*>(encoded.bytes.data());
	auto const len = static_cast<int>(encoded.bytes.size());
	auto ret = Stbi(stbi_load_from_memory(data, len, &x, &y, &ch, static_cast<int>(Image::channels_v)));
	if (ret && x > 0 && y > 0) {
		out_extent = makeExtent(x, y);
		return ret;
	}
	return {};
}

Stbi loadImage(char const* path, Extent2D& out_extent) {
	int x, y, ch;
	auto ret = Stbi(stbi_load(path, &x, &y, &ch, static_cast<int>(Image::channels_v)));
	if (ret && x > 0 && y > 0) {
		out_extent = makeExtent(x, y);
		return ret;
	}
	return {};
}
} // namespace

struct Image::Impl {
	ktl::either<Stbi, Bytes> img{};
	Extent2D extent{};
};

Image::Image() noexcept = default;

Image::Image(Image&& rhs) noexcept : Image() { swap(rhs); }

Image& Image::operator=(Image rhs) noexcept {
	swap(rhs);
	return *this;
}

Image::~Image() noexcept = default;

Image::Decoded Image::decode(Encoded encoded) {
	auto ext = Extent2D{};
	auto stbi = loadImage(encoded, ext);
	if (!stbi) { return {}; }
	auto ret = Decoded{};
	auto const size = sizeBytes(ext);
	ret.bytes = std::make_unique<std::byte[]>(size);
	std::memcpy(ret.bytes.get(), stbi.get(), size);
	return ret;
}

Extent2D Image::peek(char const* path) const {
	int x, y, ch;
	auto res = stbi_info(path, &x, &y, &ch);
	if (res == stbi_ok && x > 0 && y > 0) { return makeExtent(x, y); }
	return {};
}

Result<Extent2D> Image::load(char const* path) {
	auto ext = Extent2D{};
	if (auto stbi = loadImage(path, ext)) {
		m_impl->img = std::move(stbi);
		return m_impl->extent = ext;
	}
	return {};
}

Result<Extent2D> Image::load(Encoded image) {
	auto ext = Extent2D{};
	if (auto stbi = loadImage(std::move(image), ext)) {
		m_impl->img = std::move(stbi);
		return m_impl->extent = ext;
	}
	return {};
}

void Image::replace(Decoded image) {
	m_impl->img = std::move(image.bytes);
	m_impl->extent = image.extent;
}

std::span<std::byte const> Image::data() const {
	auto const size = sizeBytes(extent());
	if (size == 0) { return {}; }
	return m_impl->img.visit(ktl::koverloaded{
		[size](Stbi const& stbi) { return std::span(reinterpret_cast<std::byte const*>(stbi.get()), size); },
		[size](Bytes const& bytes) { return std::span(bytes.get(), size); },
	});
}

Extent2D Image::extent() const { return m_impl->extent; }
Image::View Image::view() const { return {data(), extent()}; }
Image::operator View() const { return view(); }

void Image::swap(Image& rhs) noexcept { std::swap(m_impl, rhs.m_impl); }
} // namespace vf
