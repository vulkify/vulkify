#version 450 core

layout (location = 0) in vec4 in_rgba;
layout (location = 2) in vec4 in_tint;

layout (location = 0) out vec4 out_rgba;

void main() {
	out_rgba = in_rgba * in_tint;
}
