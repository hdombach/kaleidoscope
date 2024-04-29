#version 450

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
	mat4 camera_transformation;
} global_uniform;

layout(set = 1, binding = 0) uniform ObjectUniformBuffer {
	mat4 object_transformation;
} object_uniform;


layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
	gl_Position = global_uniform.camera_transformation *
		object_uniform.object_transformation *
		vec4(inPosition, 1.0);

	fragColor = inColor;
	fragTexCoord = inTexCoord;
}
