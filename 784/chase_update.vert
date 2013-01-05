#version 330

layout (location = 0) in float type;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 pos;
layout (location = 3) in vec3 vel;
layout (location = 4) in float age;

out float gType;
out vec3 gColor;
out vec3 gPos;
out vec3 gVel;
out float gAge;

void main()
{
	gType = type;
	gColor = color;
	gPos = pos;
	gVel = vel;
	gAge = age;
}