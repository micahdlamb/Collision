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
};

uniform mat4 worldTransform;
uniform mat3 normalTransform;
uniform mat4 viewTransform;

out vec3 localPos;
out vec3 worldPos;
out vec3 normal;

void main(void)
{
	mat4 m = mat4(1);
	m[3] = vec4(0,0,-.5,1);
	m =  inverse(viewTransform) * m;

	localPos = pos;
	worldPos = vec3(m * vec4(pos,1));
	normal = normalize(normalTransform * vec3(0,0,1));
	gl_Position = eye * vec4(worldPos, 1);
}