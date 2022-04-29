#pragma once
#include <vulkify/core/geometry.hpp>
#include <vulkify/graphics/resource.hpp>
#include <type_traits>

namespace vf {
struct BufferWrite {
	void const* data{};
	std::size_t size{};
};

class GeometryBuffer : public GfxResource {
  public:
	using GfxResource::GfxResource;

	bool write(Geometry geometry);
	Geometry const& geometry() const { return m_geometry; }

  private:
	Geometry m_geometry{};
};

class UniformBuffer : public GfxResource {
  public:
	using GfxResource::GfxResource;

	std::size_t size() const;
	bool resize(std::size_t size);
	bool write(BufferWrite data);

	template <typename T>
		requires(std::is_standard_layout_v<T>)
	bool write(T const& t) { return write(std::span<T const>(&t, sizeof(T))); }
};
} // namespace vf
