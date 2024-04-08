#version 450

layout(binding = 1) uniform ColorBufferObject {
	vec3 value;
} color;
layout(binding = 2) uniform sampler2D resultTexSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(color.value, 1.0);
}
