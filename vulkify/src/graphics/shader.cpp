#include <detail/gfx_allocations.hpp>
#include <detail/spir_v.hpp>
#include <vulkify/graphics/shader.hpp>

namespace vf {
Shader::Shader() noexcept = default;
Shader::Shader(Shader&&) noexcept = default;
Shader& Shader::operator=(Shader&&) noexcept = default;
Shader::~Shader() noexcept = default;

Shader::Shader(GfxDevice const& device) {
	if (!device) { return; }
	m_module = ktl::make_unique<GfxShader>(&device);
}

bool Shader::load(std::span<std::byte const> spirv) {
	if (!m_module || !m_module->device()) { return false; }
	auto spv = SpirV::make(spirv);
	if (!spv.code.get()) { return false; }

	m_module->module = m_module->device()->device.device.createShaderModuleUnique({{}, spv.codesize, spv.code.get()});
	return static_cast<bool>(m_module->module);
}

bool Shader::load(char const* path, bool tryCompile) {
	if (!m_module || !m_module->device()) { return false; }
	auto spv = SpirV{};
	if (tryCompile) {
		spv = SpirV::load_or_compile(path);
	} else {
		spv = SpirV::load(path);
	}
	if (!spv.code.get()) { return false; }

	m_module->module = m_module->device()->device.device.createShaderModuleUnique({{}, spv.codesize, spv.code.get()});
	return static_cast<bool>(m_module->module);
}

Handle<Shader> Shader::handle() const { return {m_module.get()}; }
Shader::operator bool() const { return m_module && *m_module; }
} // namespace vf
