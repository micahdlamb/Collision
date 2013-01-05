#version 330 core

struct Light {
	mat4 eye;
	vec3 pos;
	vec3 color;
	vec4 pad[2];
};

layout(std140) uniform Global {
	mat4 eye;
	vec3 eyePos;
	int numLights;
	Light lights[6];
};

struct Ray {
	vec3 start;
	vec3 direction;
};

uniform float delta;
uniform float absorbtion;
uniform sampler3D volume;

in vec3 localPos;
in vec3 worldPos;
in vec3 normal;
in vec3 localEyePos;
out vec4 outColor;

float EPS = .001;

void main(void)
{
	Ray r;
	r.start = localPos + vec3(.5);//.5 to convert to 0-1
	r.direction = normalize(localPos - localEyePos);
	//float delta = .005;
	vec3 color = vec3(0);
	float alpha = 0;

	vec3 mover = r.start;
	vec3 forward = r.direction * delta;
	mover += forward * EPS;//for rounding errors
	//float random = fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453);
	//mover += forward * random;//dither

	while (alpha < 1 && mover.x >= 0 && mover.x <= 1 && mover.y >= 0 && mover.y <= 1 && mover.z >= 0 && mover.z <= 1){
		vec4 voxel = texture(volume, mover).rgba;
		float a = voxel.a * delta * absorbtion;
		color += voxel.rgb * a * (1-alpha);
		alpha += a;
		mover += forward;
	}

	alpha = min(alpha,1);
	outColor = vec4(color,alpha);
}