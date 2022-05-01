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

using Stbi = std::unique_ptr<stbi_uc, StbiDeleter>;
using Bytes = std::unique_ptr<std::byte[]>;
} // namespace

struct Image::Impl {
	ktl::either<Stbi, Bytes> img{};
	Extent2D extent{};
};

Image::Image() = default;

Image::Image(Image&& rhs) noexcept : Image() { swap(rhs); }

Image& Image::operator=(Image rhs) noexcept {
	swap(rhs);
	return *this;
}

Image::~Image() noexcept = default;

Image::Image(std::unique_ptr<std::byte[]> bytes, Extent2D extent) {
	m_impl->img = std::move(bytes);
	m_impl->extent = extent;
}

Result<void> Image::load(char const* file) {
	int x, y, ch;
	auto res = stbi_load(file, &x, &y, &ch, static_cast<int>(channels_v));
	if (!res) { return Error::eIOError; }
	m_impl->img = Stbi(res);
	m_impl->extent = {static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y)};
	return Result<void>::success();
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
