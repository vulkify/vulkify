#include <vulkify/context/builder.hpp>
#include <vulkify/instance/headless_instance.hpp>
#include <vulkify/instance/vf_instance.hpp>

namespace vf {
Context::Result Builder::build() {
	auto instance = UInstance{};
	m_createInfo.title = m_title.c_str();
	if (m_createInfo.instanceFlags.test(InstanceFlag::eHeadless)) {
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
