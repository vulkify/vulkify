#pragma once
#include <span>

namespace vf {
template <typename T>
concept BufferData = std::is_trivially_copyable_v<T>;

struct BufferWrite {
	void const* data;
	std::size_t size;

	constexpr BufferWrite(void const* data, std::size_t size) : data(data), size(size) {}
	template <BufferData T>
	constexpr BufferWrite(std::span<T const> bytes) : data(bytes.data()), size(bytes.size_bytes()) {}

	template <BufferData T>
	constexpr BufferWrite(T const& t) : data(&t), size(sizeof(T)) {}
};
} // namespace vf
