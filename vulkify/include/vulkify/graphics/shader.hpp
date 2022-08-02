#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <cstddef>
#include <istream>
#include <span>

namespace vf {
class Context;
struct GfxShaderModule;

class Shader {
  public:
	Shader() noexcept;
	Shader(Shader&&) noexcept;
	Shader& operator=(Shader&&) noexcept;
	~Shader() noexcept;

	Shader(Context const& context);

	explicit operator bool() const;

	bool load(std::span<std::byte const> spirv);
	bool load(char const* path, bool try_compile);

  private:
	ktl::fixed_pimpl<GfxShaderModule, 64> m_module;

	friend class Surface;
};
} // namespace vf
