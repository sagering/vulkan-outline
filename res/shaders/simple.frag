#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (set = 0, binding = 1) uniform sampler2D sampler1;

struct Ripple {
    vec3 origin;
	float t;
	float speed;
	float amplitude;
	float tdecay; // temporal decay
	float sdecay; // spatial decay
};

layout(set = 0, binding = 0) uniform global_uniform {
    mat4 vp;
	Ripple ripple[10];
} global;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 0) out vec4 outColor;

// origin
// travel speed
// amplitude
// temporal decay
// special decay

void main() {

	vec2 uv = 2 * inUV;
	
	for(int i = 0; i < 10; ++i) {
		Ripple ripple = global.ripple[i];
		
		vec3 o = ripple.origin;
		float t = ripple.t;
		float v = ripple.speed;
		float sdecay = ripple.sdecay;
		float tdecay = ripple.tdecay;
		float a = ripple.amplitude;
		
		float d = distance(o, inPos);
		float p = v * t;

		uv += normalize((inPos - o).xy) * a * exp(-pow((d - p) * sdecay, 2)) * cos(d - p) * exp(-t * tdecay);
	}
	
	vec4 val = texture(sampler1, uv);
	
	outColor.x = val.x;
	outColor.y = val.x;
	outColor.z = val.x;
	outColor.w = 1;
}