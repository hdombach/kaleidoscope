

struct Ray {
	vec4 pos;
	vec4 dir;
};

struct HitInfo {
	uint node_id;
	uint debug;
	float dist;
	int iterations;
	vec2 uv;
	vec3 normal;
	vec3 pos;
};
