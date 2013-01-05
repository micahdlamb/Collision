#version 330 core
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;

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

layout(std140) uniform Object {
	mat4 worldTransform;
	mat4 normalTransform;
};


void main(void)
{
	gl_Position = eye * worldTransform * vec4(pos, 1);
}