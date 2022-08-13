#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/graphics/handle.hpp>
#include <cstddef>
#include <istream>
#include <span>

namespace vf {
struct GfxDevice;

class Shader {
  public:
	Shader() noexcept;
	Shader(Shader&&) noexcept;
	Shader& operator=(Shader&&) noexcept;
	~Shader() noexcept;

	Shader(GfxDevice const& device);

	explicit operator bool() const;

	bool load(std::span<std::byte const> spirv);
	bool load(char const* path, bool try_compile);

	Handle<Shader> handle() const;

  private:
	ktl::kunique_ptr<class GfxShader> m_module{};

	friend class Surface;
};
} // namespace vf
