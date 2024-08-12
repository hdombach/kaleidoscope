#version 450

/*TEXTURE_UNIFORM*/
layout(set = 0, binding = 1) uniform sampler2D resultTexSampler;

layout(set = 1, binding = 0) uniform MaterialUniformBuffer {
/*MATERIAL_UNIFORM_CONTENT*/
} material_uniform;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void frag_main(/*FRAG_MAIN_ARGS*/) {
	/*FRAG_MAIN_SRC*/
}

void main() {
	/*FRAG_MAIN_CALL*/
}
