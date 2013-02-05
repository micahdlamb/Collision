#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 vNormal;

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

uniform mat4 worldTransform;
uniform mat4 inverseTransform;

out vec3 localPos;
flat out vec3 localEyePos;
flat out vec3 localLightPos;


void main(void)
{
	localPos = pos;
	vec3 worldPos = vec3(worldTransform * vec4(pos,1));
	//should be done outside of shader so only calculated once
	localEyePos = vec3(inverseTransform * vec4(eyePos, 1));
	localLightPos = vec3(inverseTransform * vec4(lights[0].pos, 1));
	gl_Position = eye * vec4(worldPos, 1);
}