#version 450 core

layout (location = 0) in vec4 in_rgba;
layout (location = 1) in vec4 in_tint;
layout (location = 2) in vec2 in_uv;

layout (std140, set = 2, binding = 0) uniform U {
	float alpha;
};

layout (set = 1, binding = 2) uniform sampler2D in_tex1;
layout (set = 2, binding = 2) uniform sampler2D in_tex2;

layout (location = 0) out vec4 out_rgba;

void main() {
	vec4 mask = alpha * texture(in_tex2, in_uv) + (1.0 - alpha) * vec4(1.0);
	out_rgba = in_tint * in_rgba * texture(in_tex1, in_uv) * mask;
}
