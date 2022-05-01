#pragma once
#include <vulkify/context/frame.hpp>
#include <vulkify/core/result.hpp>
#include <optional>

namespace vf {
class Context;
using UContext = ktl::kunique_ptr<Context>;

class Context {
  public:
	struct Info;
	using Result = vf::Result<UContext>;

	static Result make(Info info, UInstance&& instance);

	Context& operator=(Context&&) = delete;
	~Context() noexcept;

	Instance const& instance() const { return *m_instance; }

	bool closing() const { return m_instance->closing(); }
	void show() { m_instance->show(); }
	void close() { m_instance->close(); }

	Frame frame();

	Vram const& vram() const { return m_instance->vram(); }

  private:
	Context(UInstance&& instance) noexcept;

	ktl::kunique_ptr<Instance> m_instance{};
	Clock::time_point m_stamp = now();
};

struct Context::Info {};
} // namespace vf
