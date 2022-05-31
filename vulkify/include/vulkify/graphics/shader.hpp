#pragma once
#include <ktl/fixed_pimpl.hpp>
#include <vulkify/graphics/handles.hpp>
#include <span>

namespace vf {
class Context;

class Shader {
  public:
	using Handle = ShaderHandle;

	Shader() noexcept;
	Shader(Shader&&) noexcept;
	Shader& operator=(Shader&&) noexcept;
	~Shader() noexcept;

	Shader(Context const& context);

	explicit operator bool() const;

	bool load(std::span<std::byte const> spirv);
	bool load(char const* path, bool tryCompile);

	Handle handle() const;

  private:
	struct Impl;
	ktl::fixed_pimpl<Impl, 64> m_impl;
};
} // namespace vf
