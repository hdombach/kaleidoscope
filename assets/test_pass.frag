#version 450

layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out uint outNode;

void main() {
	outColor = vec4(1.0, 0.0, 0.0, 0.2);
}
