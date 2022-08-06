#pragma once
#include <vulkify/core/ptr.hpp>
#include <vulkify/graphics/texture_handle.hpp>
#include <cstring>
#include <span>
#include <vector>

namespace vf {
class Shader;

template <typename T>
concept BufferData = std::is_trivially_copyable_v<T>;

class DescriptorSet {
  public:
	DescriptorSet(Shader const& shader) : m_shader(&shader) {}

	template <BufferData T>
	void write(T const& t);
	void write(std::vector<std::byte> uniform_data) { m_data.bytes = std::move(uniform_data); }
	void write(TextureHandle texture) { m_data.texture = texture; }

  private:
	struct {
		TextureHandle texture{};
		std::vector<std::byte> bytes{};
	} m_data{};
	Ptr<Shader const> m_shader{};

	friend class Surface;
};

// impl

template <BufferData T>
void DescriptorSet::write(T const& t) {
	m_data.bytes.resize(sizeof(T));
	std::memcpy(m_data.bytes.data(), std::addressof(t), sizeof(T));
}
} // namespace vf
