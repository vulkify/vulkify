#include <detail/shared_impl.hpp>
#include <detail/spir_v.hpp>
#include <vulkify/context/context.hpp>
#include <vulkify/graphics/shader.hpp>

namespace vf {
Shader::Shader() noexcept = default;
Shader::Shader(Shader&&) noexcept = default;
Shader& Shader::operator=(Shader&&) noexcept = default;
Shader::~Shader() noexcept = default;

Shader::Shader(Context const& context) { m_impl->device = context.vram().device.device; }

Shader::operator bool() const { return static_cast<bool>(m_impl->module); }

bool Shader::load(std::span<std::byte const> spirv) {
	if (!m_impl->device) { return false; }
	auto spv = SpirV::make(spirv);
	if (!spv.code.get()) { return false; }

	m_impl->module = m_impl->device.createShaderModuleUnique({{}, spv.codesize, spv.code.get()});
	return static_cast<bool>(m_impl->module);
}

bool Shader::load(char const* path, bool tryCompile) {
	if (!m_impl->device) { return false; }
	auto spv = SpirV{};
	if (tryCompile) {
		spv = SpirV::loadOrCompile(path);
	} else {
		spv = SpirV::load(path);
	}
	if (!spv.code.get()) { return false; }

	m_impl->module = m_impl->device.createShaderModuleUnique({{}, spv.codesize, spv.code.get()});
	return static_cast<bool>(m_impl->module);
}

GfxShaderModule const& Shader::module() const { return m_impl.get(); }
} // namespace vf
