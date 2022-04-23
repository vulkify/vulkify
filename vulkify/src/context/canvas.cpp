#include <detail/canvas_impl.hpp>

namespace vf {
Canvas::Canvas() noexcept = default;
Canvas::Canvas(Impl impl) : m_impl(std::move(impl)) {}
Canvas::Canvas(Canvas&&) noexcept = default;
Canvas& Canvas::operator=(Canvas&&) noexcept = default;
Canvas::~Canvas() noexcept = default;

Canvas::operator bool() const { return m_impl->commandBuffer; }
} // namespace vf
