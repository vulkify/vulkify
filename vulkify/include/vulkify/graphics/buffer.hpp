#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/core/geometry.hpp>
#include <string>
#include <type_traits>

namespace vf {
struct GfxResource;
struct Vram;

class Buffer {
  public:
	Buffer() noexcept;
	Buffer(Buffer&&) noexcept;
	Buffer& operator=(Buffer&&) noexcept;
	virtual ~Buffer();

	Buffer(Buffer const&) = delete;
	Buffer& operator=(Buffer const&) = delete;

	Buffer(Vram const& vram, std::string name);

	explicit operator bool() const;
	GfxResource const& resource() const { return m_resource.get(); }

	void defer() &&;

  protected:
	Vram const& vram() const;

	ktl::fixed_pimpl<GfxResource, 128> m_resource;
	std::string m_name{};
};

class GeometryBuffer : public Buffer {
  public:
	GeometryBuffer() = default;
	GeometryBuffer(Vram const& vram, std::string name);

	bool write(Geometry geometry);
	Geometry const& geometry() const { return m_geometry; }

  private:
	Geometry m_geometry{};
};

class UniformBuffer : public Buffer {
  public:
	UniformBuffer() = default;
	UniformBuffer(Vram const& vram, std::string name);

	std::size_t size() const;
	bool resize(std::size_t size);
	bool write(void const* data, std::size_t size);

	template <typename T>
		requires(std::is_standard_layout_v<T>)
	bool write(T const& t) { return write(&t, sizeof(T)); }
};
} // namespace vf
