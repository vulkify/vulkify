#pragma once
#include <typeinfo>

namespace vf {
template <typename T>
concept Pointer = std::is_pointer_v<T>;

struct ErasedPtr {
	void* ptr{};
	std::size_t type{};

	ErasedPtr() = default;

	template <Pointer Type>
	constexpr ErasedPtr(Type t) : ptr(t), type(typeid(Type).hash_code()) {}

	template <Pointer Type>
	constexpr Type get() const {
		return type == typeid(Type).hash_code() ? static_cast<Type>(ptr) : nullptr;
	}

	bool operator==(ErasedPtr const&) const = default;
	explicit operator bool() const { return ptr && type != 0; }
};
} // namespace vf
