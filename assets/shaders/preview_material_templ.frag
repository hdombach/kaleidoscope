#version 450

/*TEXTURE_UNIFORM*/
layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
	/*GLOBAL_UNIFORM_CONTENT*/
} global_uniform;

layout(set = 1, binding = 0) uniform MaterialUniformBuffer {
/*MATERIAL_UNIFORM_CONTENT*/
} material_uniform;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out uint outNode;
layout(location = 2) out float outDepth;

void frag_main(/*FRAG_MAIN_ARGS*/) {
	/*FRAG_MAIN_SRC*/
}

void main() {
	/*FRAG_MAIN_CALL*/
	outNode = material_uniform.node_id; /* hard coded by ShaderResource */
	//Do I know why this works? no.
	outDepth = gl_FragCoord.z / gl_FragCoord.w / global_uniform.z_far;
}
