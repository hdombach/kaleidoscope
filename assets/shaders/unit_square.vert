#version 450

layout(location = 1) out vec2 outUV;

out gl_PerVertex
{
	vec4 gl_Position;
};

vec2 uvs[6] = vec2[6](
	vec2(0.0f, 0.0f),
	vec2(0.0f, 1.0f),
	vec2(1.0f, 0.0f),
	vec2(1.0f, 1.0f),
	vec2(1.0f, 0.0f),
	vec2(0.0f, 1.0f)
);

void main() 
{
	outUV = uvs[gl_VertexIndex];
	gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}
