#pragma once
#include <glm/vec2.hpp>
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/core/result.hpp>
#include <memory>
#include <span>

namespace vf {
using Extent = glm::uvec2;

///
/// \brief Data and extent for an image
///
template <typename T>
struct TImageData {
	T data{};
	Extent extent{};

	explicit constexpr operator bool() const { return extent.x > 0 && extent.y > 0; }
};

///
/// \brief Byte representation of an image
///
class Image {
  public:
	static constexpr std::size_t channels_v = 4;
	static constexpr std::size_t size_bytes(Extent const extent) { return extent.x * extent.y * channels_v; }

	using View = TImageData<std::span<std::byte const>>;
	///
	/// \brief Decompressed RGBA bytes
	///
	using Decoded = TImageData<std::unique_ptr<std::byte[]>>;
	struct Encoded;
	static constexpr bool valid(Extent extent) { return extent.x > 0 && extent.y > 0; }
	static constexpr bool valid(View view);

	///
	/// \brief Attempt to decompress encoded image
	///
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

///
/// \brief Compressed PNG, JPG, etc
///
struct Image::Encoded {
	std::span<std::byte const> bytes{};
};

constexpr bool Image::valid(Image::View view) { return valid(view.extent) && size_bytes(view.extent) == view.data.size_bytes(); }
} // namespace vf
