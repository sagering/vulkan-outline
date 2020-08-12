#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;

layout(set = 0, binding = 0) uniform global_uniform {
    mat4 vp;
} global;

layout(location = 0) out vec3 outPos;
layout(location = 1) out vec2 outUV;

out gl_PerVertex {
	vec4 gl_Position;
};

void main() {
    gl_Position =  global.vp * vec4(inPos, 1.0);
	outPos = inPos;
    outUV = inUV;
}