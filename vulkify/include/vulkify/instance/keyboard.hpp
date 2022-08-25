#pragma once
#include <vulkify/instance/key_event.hpp>

namespace vf::keyboard {
bool pressed(Key key);
bool held(Key key);
bool released(Key key);
bool repeated(Key key);
} // namespace vf::keyboard
