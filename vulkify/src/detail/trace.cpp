#include <detail/trace.hpp>
#include <iostream>

namespace vf {
void trace(std::string_view message) { std::cout << message; }
} // namespace vf
