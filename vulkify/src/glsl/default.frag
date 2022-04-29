#version 450 core

layout (location = 0) in vec4 in_rgba;
layout (location = 1) in vec4 in_tint;
layout (location = 2) in vec2 in_uv;

layout (set = 1, binding = 2) uniform sampler2D in_tex;

layout (location = 0) out vec4 out_rgba;

void main() {
	out_rgba = texture(in_tex, in_uv) * in_rgba * in_tint;
}
