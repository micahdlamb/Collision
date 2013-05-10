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

uniform mat4 viewTransform;

out vec3 worldPos;

void main(void)
{
	//find world position for quad that is directly in front of the viewer
	mat4 m = mat4(1);
	m[3] = vec4(0,0,-.5,1);
	m[0][0] = 100;
	m[1][1] = 100;
	m[2][2] = 100;

	m =  inverse(viewTransform) * m;

	worldPos = vec3(m * vec4(pos,1));
	gl_Position = eye * vec4(worldPos, 1);
}