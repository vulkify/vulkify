#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <span>

namespace vf {
struct GfxShaderModule;
class Context;

class Shader {
  public:
	Shader() noexcept;
	Shader(Shader&&) noexcept;
	Shader& operator=(Shader&&) noexcept;
	~Shader() noexcept;

	Shader(Context const& context);

	explicit operator bool() const;

	bool load(std::span<std::byte const> spirv);
	bool load(char const* path, bool tryCompile);

	GfxShaderModule const& module() const;

  private:
	ktl::fixed_pimpl<GfxShaderModule, 64> m_impl;
};
} // namespace vf
