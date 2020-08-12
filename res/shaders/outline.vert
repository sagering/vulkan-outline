#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPos;

layout(set = 0, binding = 0) uniform global_uniform {
    mat4 vp;
} global;


out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
	vec4 a = global.vp * vec4(inPos + inPos * 0.1, 1.0);
    gl_Position = a;
}