#include <vulkify/context/context.hpp>
#include <vulkify/instance/instance_create_info.hpp>

namespace vf {
class Builder {
  public:
	using Flag = InstanceCreateInfo::Flag;
	using Flags = InstanceCreateInfo::Flags;

	std::string const& title() const { return m_createInfo.title; }
	glm::uvec2 extent() const { return m_createInfo.extent; }
	Flags flags() const { return m_createInfo.flags; }

	Builder& setTitle(std::string set);
	Builder& setExtent(glm::uvec2 set);
	Builder& setFlags(Flags set);
	Builder& setFlag(Flag flag, bool set = true);

	Result<UContext> build();

  private:
	InstanceCreateInfo m_createInfo{};
};

// impl

inline Builder& Builder::setTitle(std::string set) {
	m_createInfo.title = std::move(set);
	return *this;
}

inline Builder& Builder::setExtent(glm::uvec2 set) {
	m_createInfo.extent = set;
	return *this;
}

inline Builder& Builder::setFlags(Flags set) {
	m_createInfo.flags = set;
	return *this;
}

inline Builder& Builder::setFlag(Flag flag, bool set) {
	m_createInfo.flags.assign(flag, set);
	return *this;
}
} // namespace vf
