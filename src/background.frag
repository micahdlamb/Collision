#version 330 core

in vec2 texCoord;
out vec4 outColor;

uniform sampler2D pic;

void main(void)
{
	outColor = texture(pic, texCoord).rgba;
}