#include <stb/stb_image.h>
#include <ktl/either.hpp>
#include <vulkify/core/unique.hpp>
#include <vulkify/graphics/image.hpp>
#include <cstring>
#include <optional>

namespace vf {
namespace {
struct StbiDeleter {
	void operator()(stbi_uc* data) const { stbi_image_free(data); }
};

static constexpr auto stbi_ok = 1;

using Stbi = std::unique_ptr<stbi_uc, StbiDeleter>;
using Bytes = std::unique_ptr<std::byte[]>;

constexpr Extent make_extent(int x, int y) { return {static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y)}; }

Stbi load_image(Image::Encoded encoded, Extent& out_extent) {
	int x, y, ch;
	auto const data = reinterpret_cast<stbi_uc const*>(encoded.bytes.data());
	auto const len = static_cast<int>(encoded.bytes.size());
	auto ret = Stbi(stbi_load_from_memory(data, len, &x, &y, &ch, static_cast<int>(Image::channels_v)));
	if (ret && x > 0 && y > 0) {
		out_extent = make_extent(x, y);
		return ret;
	}
	return {};
}

Stbi load_image(char const* path, Extent& out_extent) {
	int x, y, ch;
	auto ret = Stbi(stbi_load(path, &x, &y, &ch, static_cast<int>(Image::channels_v)));
	if (ret && x > 0 && y > 0) {
		out_extent = make_extent(x, y);
		return ret;
	}
	return {};
}
} // namespace

struct Image::Impl {
	ktl::either<Stbi, Bytes> img{};
	Extent extent{};
};

Image::Image() noexcept = default;

Image::Image(Image&& rhs) noexcept : Image() { swap(rhs); }

Image& Image::operator=(Image rhs) noexcept {
	swap(rhs);
	return *this;
}

Image::~Image() noexcept = default;

Image::Decoded Image::decode(Encoded encoded) {
	auto ext = Extent{};
	auto stbi = load_image(encoded, ext);
	if (!stbi) { return {}; }
	auto ret = Decoded{};
	auto const size = size_bytes(ext);
	ret.data = std::make_unique<std::byte[]>(size);
	std::memcpy(ret.data.get(), stbi.get(), size);
	return ret;
}

Extent Image::peek(char const* path) const {
	int x, y, ch;
	auto res = stbi_info(path, &x, &y, &ch);
	if (res == stbi_ok && x > 0 && y > 0) { return make_extent(x, y); }
	return {};
}

Result<Extent> Image::load(char const* path) {
	auto ext = Extent{};
	if (auto stbi = load_image(path, ext)) {
		m_impl->img = std::move(stbi);
		return m_impl->extent = ext;
	}
	return Error::eIOError;
}

Result<Extent> Image::load(Encoded image) {
	auto ext = Extent{};
	if (auto stbi = load_image(std::move(image), ext)) {
		m_impl->img = std::move(stbi);
		return m_impl->extent = ext;
	}
	return Error::eInvalidArgument;
}

void Image::replace(Decoded image) {
	m_impl->img = std::move(image.data);
	m_impl->extent = image.extent;
}

std::span<std::byte const> Image::data() const {
	auto const size = size_bytes(extent());
	if (size == 0) { return {}; }
	return m_impl->img.visit(ktl::koverloaded{
		[size](Stbi const& stbi) { return std::span(reinterpret_cast<std::byte const*>(stbi.get()), size); },
		[size](Bytes const& bytes) { return std::span(bytes.get(), size); },
	});
}

Extent Image::extent() const { return m_impl->extent; }
Image::View Image::view() const { return {data(), extent()}; }
Image::operator View() const { return view(); }

void Image::swap(Image& rhs) noexcept { std::swap(m_impl, rhs.m_impl); }
} // namespace vf
