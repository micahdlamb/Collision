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

uniform mat4 worldTransform;
uniform mat3 normalTransform;

out vec3 localPos;
out vec3 worldPos;
out vec3 normal;

void main(void)
{
	localPos = pos;
	worldPos = vec3(worldTransform * vec4(pos,1));
	normal = normalize(normalTransform * norm);
	gl_Position = eye * vec4(worldPos, 1);
}