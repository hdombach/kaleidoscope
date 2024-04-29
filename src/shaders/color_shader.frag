#version 450

layout(set = 0, binding = 1) uniform sampler2D resultTexSampler;

layout(set = 1, binding = 0) uniform ColorBufferObject {
	mat4 object_transformation;
	vec3 color;
} uniform_buffer;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(uniform_buffer.color, 1.0);
}
