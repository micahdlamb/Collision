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
uniform mat3 normalTransform;

out vec3 localPos;
out vec3 worldPos;
out vec3 normal;
out vec3 localEyePos;

void main(void)
{
	localPos = pos;
	worldPos = vec3(worldTransform * vec4(pos,1));
	normal = normalize(normalTransform * vNormal);
	localEyePos = vec3(inverseTransform * vec4(eyePos, 1));//should be calculated outside of this shader eventually
	gl_Position = eye * vec4(worldPos, 1);
}