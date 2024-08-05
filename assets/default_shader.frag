#version 450

/*TEXTURE_UNIFORM*/
layout(set = 0, binding = 1) uniform sampler2D resultTexSampler;

layout(set = 1, binding = 0) uniform MaterialUniformBuffer {
/*INSERT_MATERIAL_UNIFORM*/
} material_uniform;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void frag_main(/*SHADER_ARGS*/) {
	/*INSERT_FRAG_SRC*/
}

void main() {
	/*FRAG_MAIN*/
}
