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

	bool load(std::span<std::byte const> spirv);
	bool load(char const* path, bool try_compile);

	Handle<Shader> handle() const;

	explicit operator bool() const;

  private:
	ktl::kunique_ptr<class GfxShader> m_module{};
};
} // namespace vf
