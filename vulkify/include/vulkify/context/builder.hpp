#include <vulkify/context/context.hpp>
#include <vulkify/instance/instance_create_info.hpp>
#include <string>

namespace vf {
///
/// \brief Context (and Instance) builder
///
class Builder {
  public:
	std::string const& title() const { return m_title; }
	glm::uvec2 extent() const { return m_createInfo.extent; }
	WindowFlags windowFlags() const { return m_createInfo.windowFlags; }
	InstanceFlags instanceFlags() const { return m_createInfo.instanceFlags; }
	AntiAliasing antiAliasing() const { return m_createInfo.desiredAA; }

	Builder& setTitle(std::string set);
	Builder& setExtent(glm::uvec2 set);
	Builder& updateFlags(WindowFlags set, WindowFlags unset = {});
	Builder& updateFlags(InstanceFlags set, InstanceFlags unset = {});
	Builder& setFlag(WindowFlag flag, bool set = true);
	Builder& setFlag(InstanceFlag flag, bool set = true);
	Builder& setAntiAliasing(AntiAliasing aa);

	Context::Result build();

  private:
	InstanceCreateInfo m_createInfo{};
	std::string m_title{"(Untitled)"};
};

// impl

inline Builder& Builder::setTitle(std::string set) {
	m_title = std::move(set);
	return *this;
}

inline Builder& Builder::setExtent(glm::uvec2 set) {
	m_createInfo.extent = set;
	return *this;
}

inline Builder& Builder::updateFlags(WindowFlags set, WindowFlags unset) {
	m_createInfo.windowFlags.update(set, unset);
	return *this;
}

inline Builder& Builder::updateFlags(InstanceFlags set, InstanceFlags unset) {
	m_createInfo.instanceFlags.update(set, unset);
	return *this;
}

inline Builder& Builder::setFlag(WindowFlag flag, bool set) {
	m_createInfo.windowFlags.assign(flag, set);
	return *this;
}

inline Builder& Builder::setFlag(InstanceFlag flag, bool set) {
	m_createInfo.instanceFlags.assign(flag, set);
	return *this;
}

inline Builder& Builder::setAntiAliasing(AntiAliasing aa) {
	m_createInfo.desiredAA = aa;
	return *this;
}
} // namespace vf
