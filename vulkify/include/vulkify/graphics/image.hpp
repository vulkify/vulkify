#pragma once
#include <glm/vec2.hpp>
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/core/result.hpp>
#include <memory>
#include <span>

namespace vf {
using Extent2D = glm::uvec2;

class Image {
  public:
	static constexpr std::size_t channels_v = 4;
	static constexpr std::size_t sizeBytes(Extent2D const extent) { return extent.x * extent.y * channels_v; }

	struct View;
	static constexpr bool valid(View view);

	Image();
	Image(Image&&) noexcept;
	Image(Image const&);
	Image& operator=(Image) noexcept;
	~Image() noexcept;

	Image(std::unique_ptr<std::byte[]> bytes, Extent2D extent);

	Result<void> load(char const* file);

	std::span<std::byte const> data() const;
	Extent2D extent() const;
	View view() const;

	operator View() const;

  private:
	void swap(Image& rhs) noexcept;

	struct Impl;
	ktl::fixed_pimpl<Impl, 32> m_impl;
};

struct Image::View {
	std::span<std::byte const> bytes{};
	Extent2D extent{};
};

constexpr bool Image::valid(Image::View view) { return view.extent.x > 0 && view.extent.y > 0 && sizeBytes(view.extent) == view.bytes.size_bytes(); }
} // namespace vf
