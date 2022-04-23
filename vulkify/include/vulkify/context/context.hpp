#pragma once
#include <vulkify/core/result.hpp>
#include <vulkify/instance/instance.hpp>

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

	bool isOpen() const { return m_instance->isOpen(); }
	void show() { m_instance->show(); }
	void close() { m_instance->close(); }
	Instance::Poll poll() { return m_instance->poll(); }

  private:
	Context(UInstance&& instance) noexcept;

	ktl::kunique_ptr<Instance> m_instance{};
};

struct Context::Info {};
} // namespace vf
