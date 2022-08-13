#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/graphics/handle.hpp>
#include <cstddef>
#include <istream>
#include <span>

namespace vf {
class Context;
namespace refactor {
struct GfxShaderModule;
class Surface;
} // namespace refactor

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
	ktl::kunique_ptr<refactor::GfxShaderModule> m_module{};

	friend class refactor::Surface;
};
} // namespace vf
