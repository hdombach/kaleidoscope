#version 450

layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;
layout(location = 1) out uint outNode;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer {
	/*GLOBAL_UNIFORM_CONTENT*/
} global_uniform;

layout(set = 1, binding = 0) uniform sampler2D depthSampler;


/*UNIFORM_DECL*/

/*DE_FUNC*/

float de(vec3 pos) {
	vec3 z = pos;
	float dr = 1;
	float r = 0;
	float power = 8.0;
	float bailout = 2.0f;
	uint iterations = 20;
	float orbitLife;
	for (uint i = 0; i < iterations; i++) {
		r = length(z);
		if (r > bailout) {
			orbitLife = i;
			break;
		}
		//convert to polar
		float theta = acos(z.z / r);
		float phi = atan(z.y / z.x);
		dr = pow(r, power - 1) * power * dr + 1;
		//scale and rotate the point
		float zr = pow(r, power);
		theta = theta * power;
		phi = phi * power;
		//convert back to cartesian
		z = zr * vec3(sin(theta) * cos(phi), sin(phi) * sin(theta), cos(theta));
		z += pos;
		orbitLife = i;
	}
	orbitLife = orbitLife / iterations;
	return 0.5 * log(r) * r / dr;
}

bool de_intersect(vec3 pos, vec3 dir, inout float d, inout int iterations, inout vec3 closest_pos) {
	float step, smallest = 1000;
	iterations = 0;
	d = 0;
	do {
		step = de(pos);
		if (step < 0.0001 || iterations > 40) {
			return true;
		}
		iterations++;
		pos += dir * step;
		if (step < smallest) {
			smallest = step;
			closest_pos = pos;
		}
		d += step;
		if (length(pos) > 2.0) return false;
	} while (true);
	return false;
}


float PI = 3.14159265358979;

void main() {
	vec2 uv = fragTexCoord;

	vec4 dir = vec4(1.0 - uv.x * 2.0, uv.y * 2.0 - 1.0, 2.0, 0.0);
	dir.x *= global_uniform.aspect;
	dir.z = 1.0 / tan(global_uniform.fovy * PI / 360.0f);
	dir = normalize(dir) * -1 * global_uniform.camera_rotation;
	vec2 intr_uv;
	vec2 tex_uv;
	uint node_id = 0;

	vec4 position = vec4(0, 0, 0, 1) - global_uniform.camera_translation;


	uint debug = 0;
	vec3 normal;

	vec4 color;
	vec4 hit_pos = vec4(0.0, 0.0, 0.0, 1.0);

	int iterations;
	float d = 0;
	float cur_depth = texture(depthSampler, uv).x;
	if (de_intersect(position.xyz, dir.xyz, d, iterations, hit_pos.xyz)) {

		hit_pos.w = 1.0;
		hit_pos = global_uniform.camera_transformation * hit_pos;
		d = hit_pos.z / global_uniform.z_far;

		if (cur_depth > d) {
			color.xyz = vec3(float(iterations) / 40.0);
			color.w = 1.0;
		} else {
			color.w = 0.0;
		}
	} else {
		color = vec4(0.0, 0.0, 0, 0.0);
	}
	outColor = color;
}
