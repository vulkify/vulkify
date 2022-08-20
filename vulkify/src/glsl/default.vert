#version 450 core

struct Model {
	vec4 pos_orn;
	vec4 scl_z_tint;
};

layout (location = 0) in vec2 v_pos;
layout (location = 1) in vec2 v_uv;
layout (location = 2) in vec4 v_rgba;

layout (std140, set = 0, binding = 0) uniform P {
	mat4 mat_p;
};

layout (std140, set = 1, binding = 0) uniform V {
	Model view;
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

mat4 make_mat(Model model) {
	vec3 pos = vec3(model.pos_orn.xy, model.scl_z_tint.z);
	mat4 t = mat4(
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		pos.x, pos.y, pos.z, 1.0
	);

	vec2 orn = model.pos_orn.zw;
	mat3 r = mat3(
		orn.x, orn.y, 0.0,
		-orn.y, orn.x, 0.0,
		0.0, 0.0, 1.0
	);
	
	vec2 scl = model.scl_z_tint.xy;
	mat3 s = mat3(
		scl.x, 0.0, 0.0,
		0.0, scl.y, 0.0,
		0.0, 0.0, 1.0
	);

	return t * mat4(r) * mat4(s);
}

void main() {
	f_uv = v_uv;
	f_rgba = v_rgba;
	
	Model m = model[gl_InstanceIndex];
	uint tint = floatBitsToUint(m.scl_z_tint.w);
	float tr = float((tint >> 24) & 0xff) / 255.0;
	float tg = float((tint >> 16) & 0xff) / 255.0;
	float tb = float((tint >>  8) & 0xff) / 255.0;
	float ta = float((tint >>  0) & 0xff) / 255.0;
	f_tint = vec4(tr, tg, tb, ta);

	mat4 mat_v = make_mat(view);
	mat4 mat_m = make_mat(m);

	gl_Position = mat_p * mat_v * mat_m * vec4(v_pos, 0.0, 1.0);
}
