#version 450

layout(binding = 1) uniform sampler2D texSampler;
layout(binding = 2) uniform sampler2D resultTexSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = texture(texSampler, fragTexCoord);
	outColor.xyz = vec3(pow(outColor.x, 1/2.2), pow(outColor.y, 1/2.2), pow(outColor.z, 1/2.2));
	//outColor += texture(resultTexSampler, fragTexCoord) / 10;
	vec4 randColor[1];
	//int index = int(fragTexCoord.x * fragTexCoord.y * 1000);
	//outColor = randColor[index];
}
