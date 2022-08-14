#pragma once
#include <ktl/not_null.hpp>
#include <vulkify/core/ptr.hpp>
#include <vulkify/graphics/handle.hpp>
#include <cstring>
#include <span>
#include <vector>

namespace vf {
class Shader;
class Surface;
class Texture;

template <typename T>
concept BufferData = std::is_trivially_copyable_v<T>;

class DescriptorSet {
  public:
	explicit DescriptorSet(ktl::not_null<Shader const*> shader);
	explicit DescriptorSet(Handle<Shader> shader) : m_shader(shader) {}

	template <BufferData T>
	void write(T const& t);
	void write(std::vector<std::byte> uniform_data) { m_data.bytes = std::move(uniform_data); }
	void write(Handle<Texture> texture) { m_data.texture = texture; }

  private:
	struct {
		Handle<Texture> texture{};
		std::vector<std::byte> bytes{};
	} m_data{};
	Handle<Shader> m_shader{};

	friend class Surface;
	friend class Surface;
};

// impl

template <BufferData T>
void DescriptorSet::write(T const& t) {
	m_data.bytes.resize(sizeof(T));
	std::memcpy(m_data.bytes.data(), std::addressof(t), sizeof(T));
}
} // namespace vf
