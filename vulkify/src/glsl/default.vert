#version 450 core

struct Model {
	vec4 pos_rot;
	vec4 scl;
	vec4 tint;
};

struct View {
	mat4 mat_v;
	mat4 mat_p;
};

layout (location = 0) in vec2 v_pos;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec4 v_rgba;

layout (std140, set = 0, binding = 0) uniform V {
	View view;
};

layout (std430, set = 1, binding = 1) readonly buffer M {
	Model model[];
};

layout (location = 0) out vec4 f_rgba;
layout (location = 1) out vec2 f_uv;
layout (location = 2) out vec4 f_tint;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	f_uv = v_uv;
	f_rgba = v_rgba;
	
	Model m = model[gl_InstanceIndex];
	f_tint = m.tint;
	
	vec2 pos = m.pos_rot.xy;
	vec2 rot = m.pos_rot.zw;

	mat3 t = mat3(
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		pos.x, pos.y, 1.0
	);

	mat3 r = mat3(
		rot.x, rot.y, 0.0,
		-rot.y, rot.x, 0.0,
		0.0, 0.0, 1.0
	);
	
	mat3 s = mat3(
		m.scl.x, 0.0, 0.0,
		0.0, m.scl.y, 0.0,
		0.0, 0.0, 1.0
	);

	vec3 pos3 = t * r * s * vec3(v_pos, 1.0);
	gl_Position = view.mat_p * view.mat_v * vec4(pos3, 1.0);
}
