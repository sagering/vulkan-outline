#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform global_uniform {
    mat4 vp;
	mat4 m;
	ivec2 res;
} global;

layout(set = 0, binding = 1) uniform sampler2D sampler1;

layout(location = 0) out vec4 outColor;

// kernel dimensions
const int halfW = 4;
const int halfH = 4;

vec4 simpleEdgeFilter(vec4 edgeColor) {
	ivec2 pixelPos = ivec2(gl_FragCoord.xy);
	vec4 pixelColor = texelFetch(sampler1, pixelPos, 0);
	
	if(pixelColor.w == 0) return pixelColor;
	
	for(int i = -halfW; i < halfW + 1; ++i) {
		for(int j = -halfH; j < halfH + 1; ++j) {
			ivec2 pos = pixelPos + ivec2(i, j);
			if (pos.x < 0 || pos.x >= global.res.x) continue;
			if (pos.y < 0 || pos.y >= global.res.y) continue;
			
			if(texelFetch(sampler1, pos, 0).w == 0) return edgeColor;
		}
	}
	
	return pixelColor;
}

vec4 passThrough() {
	return texelFetch(sampler1, ivec2(gl_FragCoord.xy), 0);
}

void main() {
	outColor =  simpleEdgeFilter(vec4(1, 1, 1, 1));
}