#version 450 core

struct Model {
	vec4 pos_orn;
	vec4 scl_tint;
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
layout (location = 1) out vec4 f_tint;
layout (location = 2) out vec2 f_uv;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	f_uv = v_uv;
	f_rgba = v_rgba;
	
	Model m = model[gl_InstanceIndex];
	uint tint = floatBitsToUint(m.scl_tint.z);
	float tr = float((tint >> 24) & 0xff) / 255.0;
	float tg = float((tint >> 16) & 0xff) / 255.0;
	float tb = float((tint >>  8) & 0xff) / 255.0;
	float ta = float((tint >>  0) & 0xff) / 255.0;
	f_tint = vec4(tr, tg, tb, ta);
	
	vec2 pos = m.pos_orn.xy;
	mat3 t = mat3(
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		pos.x, pos.y, 1.0
	);

	vec2 orn = m.pos_orn.zw;
	mat3 r = mat3(
		orn.x, orn.y, 0.0,
		-orn.y, orn.x, 0.0,
		0.0, 0.0, 1.0
	);
	
	vec2 scl = m.scl_tint.xy;
	mat3 s = mat3(
		m.scl_tint.x, 0.0, 0.0,
		0.0, m.scl_tint.y, 0.0,
		0.0, 0.0, 1.0
	);

	vec3 pos3 = t * r * s * vec3(v_pos, 1.0);
	gl_Position = view.mat_p * view.mat_v * vec4(pos3, 1.0);
}
