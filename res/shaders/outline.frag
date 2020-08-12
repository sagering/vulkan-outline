#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

void main() {

	outColor.x = 1.0;
	outColor.y = 0.0;
	outColor.z = 0.0;
	outColor.w = 1;
}