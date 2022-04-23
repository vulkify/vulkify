#include <detail/trace.hpp>
#include <iostream>

namespace vf {
void trace(std::string message) {
	message += '\n';
	std::cout << message;
}
} // namespace vf
