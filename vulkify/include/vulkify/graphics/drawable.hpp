#pragma once
#include <ktl/kunique_ptr.hpp>
#include <vulkify/graphics/primitive.hpp>

namespace vf {
template <typename T>
concept PrimitiveApi = requires(T const& t) {
	{ t.getPrimitive() } -> std::convertible_to<Primitive>;
};

class Drawable {
  public:
	template <PrimitiveApi T = Primitive>
	explicit Drawable(T t = {}) : m_model(ktl::make_unique<Model<T>>(std::move(t))) {}

	template <PrimitiveApi T>
	T* as() const;

	void draw(Surface const& surface) const { m_model->primitive().draw(surface); }

  private:
	struct Base {
		virtual ~Base() = default;
		virtual Primitive primitive() const = 0;
	};
	template <PrimitiveApi T>
	struct Model : Base {
		T t;
		template <typename... Args>
		Model(Args&&... args) : t(std::forward<Args>(args)...) {}
		Primitive primitive() const override { return t.getPrimitive(); }
	};
	ktl::kunique_ptr<Base> m_model;
};

// impl

template <PrimitiveApi T>
T* Drawable::as() const {
	auto t = dynamic_cast<Model<T>*>(m_model.get());
	return t ? &t->t : nullptr;
}
} // namespace vf
