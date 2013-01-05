#version 410 core

in float fZ;

out vec4 outColor;

void main(void)
{
	float z = fZ*.5 + .5;
	float moment1 = z;
	float moment2 = z*z;

	float dx = dFdx(z);
	float dy = dFdy(z);
	moment2 += .25 * (dx*dx + dy*dy);

	outColor = vec4( moment1,moment2, 0.0, 1 );
}