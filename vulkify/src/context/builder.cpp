#include <vulkify/context/builder.hpp>
#include <vulkify/instance/headless_instance.hpp>
#include <vulkify/instance/vf_instance.hpp>

namespace vf {
Result<UContext> Builder::build() {
	auto instance = UInstance{};
	if (m_createInfo.flags.test(Flag::eHeadless)) {
		auto inst = HeadlessInstance::make();
		if (!inst) { return inst.error(); }
		instance = std::move(inst.value());
	} else {
		auto inst = VulkifyInstance::make(m_createInfo);
		if (!inst) { return inst.error(); }
		instance = std::move(inst.value());
	}
	if (!instance) { return Error::eUnknown; }
	return Context::make(std::move(instance));
}
} // namespace vf
