#include <vulkify/context/context.hpp>
#include <vulkify/instance/instance_create_info.hpp>
#include <functional>
#include <string>

namespace vf {
using SelectGpu = std::function<std::size_t(std::span<Gpu const>)>;

///
/// \brief Context (and Instance) builder
///
class Builder {
  public:
	std::string const& title() const { return m_title; }
	glm::uvec2 extent() const { return m_create_info.extent; }
	WindowFlags window_flags() const { return m_create_info.window_flags; }
	InstanceFlags instance_flags() const { return m_create_info.instance_flags; }
	AntiAliasing anti_aliasing() const { return m_create_info.desired_aa; }
	std::span<VSync const> vsyncs() const { return m_create_info.desired_vsyncs; }
	ZOrder default_z_order() const { return m_create_info.default_z_order; }

	Builder& set_title(std::string set);
	Builder& set_extent(glm::uvec2 set);
	Builder& update_flags(WindowFlags set, WindowFlags unset = {});
	Builder& update_flags(InstanceFlags set, InstanceFlags unset = {});
	Builder& set_flag(WindowFlag flag, bool set = true);
	Builder& set_flag(InstanceFlag flag, bool set = true);
	Builder& set_anti_aliasing(AntiAliasing aa);
	Builder& set_select_gpu(SelectGpu select_gpu);
	Builder& set_vsyncs(std::vector<VSync> desired);
	Builder& set_default_z_order(ZOrder z_order);

	Context::Result build();

  private:
	SelectGpu m_selec_gGpu{};
	InstanceCreateInfo m_create_info{};
	std::string m_title{"(Untitled)"};
};

// impl

inline Builder& Builder::set_title(std::string set) {
	m_title = std::move(set);
	return *this;
}

inline Builder& Builder::set_extent(glm::uvec2 set) {
	m_create_info.extent = set;
	return *this;
}

inline Builder& Builder::update_flags(WindowFlags set, WindowFlags unset) {
	m_create_info.window_flags.update(set, unset);
	return *this;
}

inline Builder& Builder::update_flags(InstanceFlags set, InstanceFlags unset) {
	m_create_info.instance_flags.update(set, unset);
	return *this;
}

inline Builder& Builder::set_flag(WindowFlag flag, bool set) {
	m_create_info.window_flags.assign(flag, set);
	return *this;
}

inline Builder& Builder::set_flag(InstanceFlag flag, bool set) {
	m_create_info.instance_flags.assign(flag, set);
	return *this;
}

inline Builder& Builder::set_anti_aliasing(AntiAliasing aa) {
	m_create_info.desired_aa = aa;
	return *this;
}

inline Builder& Builder::set_select_gpu(SelectGpu selectGpu) {
	m_selec_gGpu = std::move(selectGpu);
	return *this;
}

inline Builder& Builder::set_vsyncs(std::vector<VSync> desired) {
	m_create_info.desired_vsyncs = std::move(desired);
	return *this;
}

inline Builder& Builder::set_default_z_order(ZOrder z_order) {
	m_create_info.default_z_order = z_order;
	return *this;
}
} // namespace vf
