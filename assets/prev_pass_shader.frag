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

void frag_main(/*FRAG_MAIN_ARGS*/) {
	/*FRAG_MAIN_SRC*/
}

float linearDepth(float depth)
{
	float z = depth * 2.0f - 1.0f; 
	return (2.0f * global_uniform.z_near * global_uniform.z_far) / (global_uniform.z_far + global_uniform.z_near - z * (global_uniform.z_far - global_uniform.z_near));	
}

void main() {
	/*FRAG_MAIN_CALL*/
	outColor.r = linearDepth(gl_FragCoord.z);
	outNode = material_uniform.node_id; /* hard coded by ShaderResource */
}
