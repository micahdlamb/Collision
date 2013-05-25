#version 330 core
layout(location = 0) in vec3 pos;

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

uniform mat4 transform;

out vec3 worldPos;

void main(void)
{
	worldPos = vec3(transform * vec4(pos,1));
	gl_Position = eye * vec4(worldPos, 1);
}