#pragma once
#include <vulkify/core/result.hpp>
#include <vulkify/core/time.hpp>
#include <vulkify/instance/instance.hpp>
#include <optional>

namespace vf {
class Context;
using UContext = ktl::kunique_ptr<Context>;

struct Frame {
	Instance::Poll poll{};
	Canvas canvas{};
	Time dt{};
};

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
	Instance::Poll poll() { return m_instance->poll(); }

	Frame nextFrame();
	bool submit(Rgba clear);

  private:
	Context(UInstance&& instance) noexcept;

	ktl::kunique_ptr<Instance> m_instance{};
	Clock::time_point m_stamp = now();
	bool m_ready{};
};

struct Context::Info {};
} // namespace vf
