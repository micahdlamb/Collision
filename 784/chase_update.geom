#version 330

layout(points) in;
layout(points) out;
layout(max_vertices = 113) out;

in float gType[];
in vec3 gColor[];
in vec3 gPos[];
in vec3 gVel[];
in float gAge[];

out float oType;
out vec3 oColor;
out vec3 oPos;
out vec3 oVel;
out float oAge;

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
	float time;
	float deltaTime;
	vec2 screenDim;
};

uniform float acc;
uniform float maxSpeed;
uniform vec3 chasePt;
uniform sampler1D randDirs;

#define EMITTER 0
#define NORMAL 1
#define EMITTER_LIFETIME .005
#define NORMAL_LIFETIME 100000

vec3 getRandomDir(float texCoord)
{
    vec3 dir = texture(randDirs, texCoord).xyz;
    dir -= vec3(0.5, 0.5, 0.5);
    return normalize(dir);
}

void main()
{
    float age = gAge[0] + deltaTime;

	if (gType[0] == EMITTER){
		if (age > EMITTER_LIFETIME){
			age = 0;
			vec3 dir = getRandomDir(time);

			oType = NORMAL;
			oColor = dir*.5 + .5;
			oPos = gPos[0];
			oVel = dir * acc;
			oAge = 0;
			EmitVertex();
			EndPrimitive();
		}

		oType = EMITTER;
		oColor = gColor[0];
		oPos = gPos[0];
		oVel = gVel[0];
		oAge = age;
		EmitVertex();
		EndPrimitive();
	} else {
		if (age < NORMAL_LIFETIME){
			vec3 x = normalize(chasePt - gPos[0]) * acc;

			oType = NORMAL;
			oColor = gColor[0];
			oVel = gVel[0] + x * deltaTime;
			oPos = gPos[0] + (oVel + gVel[0]) * .5 * deltaTime;
			float newSpeed = length(oVel);
			if (newSpeed > maxSpeed)
				oVel *= maxSpeed / newSpeed;
			oAge = age;
			EmitVertex();
			EndPrimitive();
		}
	}
}