#version 330 core
uniform int id;

out uvec3 outColor;

void main(void)
{
	outColor = uvec3(id,0,0);
}