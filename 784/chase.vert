#version 330

layout (location = 0) in float type;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 pos;
layout (location = 3) in vec3 vel;
layout (location = 4) in float age;

layout(std140) uniform Object {
	mat4 worldTransform;
	mat4 normalTransform;
};

out vec3 gColor;

void main()
{
	gColor = color;
	gl_Position = worldTransform * vec4(pos, 1.0);
}
