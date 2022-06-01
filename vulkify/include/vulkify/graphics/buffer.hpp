#pragma once
#include <vulkify/core/result.hpp>
#include <vulkify/graphics/geometry.hpp>
#include <vulkify/graphics/resource.hpp>

namespace vf {
class Context;

template <typename T>
concept BufferData = std::is_trivially_copyable_v<T>;

///
/// \brief GPU Vertex (and index) buffer
///
class GeometryBuffer : public GfxResource {
  public:
	struct Counts {
		std::uint32_t vertices{};
		std::uint32_t indices{};
	};

	GeometryBuffer() = default;
	GeometryBuffer(Context const& context, std::string name);

	Result<void> write(Geometry geometry);

	Geometry geometry() const;
	Counts counts() const { return m_counts; }

  private:
	Counts m_counts{};
};

///
/// \brief GPU Uniform buffer
///
class UniformBuffer : public GfxResource {
  public:
	UniformBuffer() = default;
	UniformBuffer(Context const& context, std::string name);

	std::size_t size() const;
	Result<void> reserve(std::size_t size);

	Result<void> write(void const* data, std::size_t size);

	template <BufferData T>
	Result<void> write(std::span<T const> data) {
		return write(data.data(), data.size_bytes());
	}
};
} // namespace vf
