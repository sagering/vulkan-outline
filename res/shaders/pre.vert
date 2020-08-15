#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPos;

layout(set = 0, binding = 0) uniform global_uniform {
    mat4 vp;
	mat4 m;
} global;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
    gl_Position =  global.vp * global.m * vec4(inPos, 1.0);
}