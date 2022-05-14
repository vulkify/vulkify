#pragma once
#include <glm/vec2.hpp>
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/core/result.hpp>
#include <memory>
#include <span>

namespace vf {
using Extent = glm::uvec2;

template <typename T>
struct TImageData {
	T data{};
	Extent extent{};

	explicit constexpr operator bool() const { return extent.x > 0 && extent.y > 0; }
};

class Image {
  public:
	static constexpr std::size_t channels_v = 4;
	static constexpr std::size_t sizeBytes(Extent const extent) { return extent.x * extent.y * channels_v; }

	using View = TImageData<std::span<std::byte const>>;
	using Decoded = TImageData<std::unique_ptr<std::byte[]>>;
	struct Encoded;
	static constexpr bool valid(Extent extent) { return extent.x > 0 && extent.y > 0; }
	static constexpr bool valid(View view);

	static Decoded decode(Encoded encoded);

	Image() noexcept;
	Image(Image&&) noexcept;
	Image(Image const&);
	Image& operator=(Image) noexcept;
	~Image() noexcept;

	Extent peek(char const* path) const;
	Result<Extent> load(char const* path);
	Result<Extent> load(Encoded image);
	void replace(Decoded image);

	std::span<std::byte const> data() const;
	Extent extent() const;
	View view() const;

	operator View() const;

  private:
	void swap(Image& rhs) noexcept;

	struct Impl;
	ktl::fixed_pimpl<Impl, 32> m_impl;
};

struct Image::Encoded {
	std::span<std::byte const> bytes{};
};

constexpr bool Image::valid(Image::View view) { return valid(view.extent) && sizeBytes(view.extent) == view.data.size_bytes(); }
} // namespace vf
