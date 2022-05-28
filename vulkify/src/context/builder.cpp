#include <detail/trace.hpp>
#include <vulkify/context/builder.hpp>
#include <vulkify/instance/headless_instance.hpp>
#include <vulkify/instance/vf_instance.hpp>

namespace vf {
namespace {
struct Selector : GpuSelector {
	SelectGpu select{};
	Selector(SelectGpu select) : select(std::move(select)) {}
	Gpu const* operator()(Gpu const* first, Gpu const* last) const override {
		auto const size = static_cast<std::size_t>(last - first);
		auto ret = select({first, size});
		if (ret >= size) {
			VF_TRACEF("[vf::(internal)] Out-of-range Gpu index [{}], max: {}", ret, size - 1);
			ret = 0;
		}
		return first + ret;
	}
};
} // namespace

Context::Result Builder::build() {
	auto instance = UInstance{};
	auto selector = Selector(std::move(m_selectGpu));
	m_createInfo.title = m_title.c_str();
	if (selector.select) { m_createInfo.gpuSelector = &selector; }
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
