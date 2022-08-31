#pragma once
#include <glm/vec2.hpp>
#include <vulkify/instance/key_event.hpp>

namespace vf::mouse {
bool pressed(Key key);
bool held(Key key);
bool released(Key key);
bool repeated(Key key);
glm::vec2 position();
glm::vec2 scroll();
} // namespace vf::mouse
