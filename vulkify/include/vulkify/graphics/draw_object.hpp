#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/core/transform.hpp>
#include <vulkify/graphics/buffer.hpp>
#include <vulkify/graphics/draw_model.hpp>

namespace vf {
class Surface;

struct DrawInstance {
	Transform transform{};
	Rgba tint = white_v;

	DrawModel model() const;
};

template <typename T>
concept Drawable = requires(T const& t) {
	{ t.geometryBuffer() } -> std::convertible_to<GeometryBuffer const&>;
	{ t.drawInstances() } -> std::convertible_to<std::span<DrawInstance const>>;
};

struct DrawPrimitive {
	DrawInstance instance{};
	GeometryBuffer buffer{};

	static DrawPrimitive make(Vram const& vram, std::string name) { return {{}, {vram, std::move(name)}}; }

	GeometryBuffer const& geometryBuffer() const { return buffer; }
	std::span<DrawInstance const> drawInstances() const { return {&instance, 1}; }
};

template <typename Storage = std::vector<DrawInstance>>
struct InstancedPrimitive {
	Storage instances{};
	GeometryBuffer buffer{};

	static InstancedPrimitive make(Vram const& vram, std::string name) { return {{}, {vram, std::move(name)}}; }

	GeometryBuffer const& geometryBuffer() const { return buffer; }
	std::span<DrawInstance const> drawInstances() const { return {instances.data(), instances.size()}; }
};

class DrawObject {
  public:
	static constexpr std::size_t small_buffer_v = 8;

	template <Drawable T = DrawPrimitive>
	explicit DrawObject(T t = {}) : m_model(ktl::make_unique<Model<T>>(std::move(t))) {}

	template <Drawable T>
	T* as() const;

	void draw(Surface const& surface) const;

  private:
	struct Base {
		virtual ~Base() = default;
		virtual GeometryBuffer const& geometry() const = 0;
		virtual std::span<DrawInstance const> instances() const = 0;
	};
	template <Drawable T>
	struct Model : Base {
		T t;
		template <typename... Args>
		Model(Args&&... args) : t(std::forward<Args>(args)...) {}
		GeometryBuffer const& geometry() const { return t.geometryBuffer(); }
		std::span<DrawInstance const> instances() const { return t.drawInstances(); }
	};
	ktl::kunique_ptr<Base> m_model;
};

// impl

template <Drawable T>
T* DrawObject::as() const {
	auto t = dynamic_cast<Model<T>*>(m_model.get());
	return t ? &t->t : nullptr;
}

inline DrawModel DrawInstance::model() const {
	auto ret = DrawModel{};
	ret.pos_orn = {transform.position, static_cast<glm::vec2>(transform.orientation)};
	auto const utint = tint.toU32();
	ret.scl_tint = {transform.scale, *reinterpret_cast<float const*>(&utint), 0.0f};
	return ret;
}
} // namespace vf
