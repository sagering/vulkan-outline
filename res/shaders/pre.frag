#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform global_uniform {
    mat4 vp;
	mat4 m;
	ivec2 res;
} global;

void main() {
	outColor = vec4(gl_FragCoord.x / (global.res.x - 1), 0, 1, 1);
}