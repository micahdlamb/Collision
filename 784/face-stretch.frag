#version 330 core

in vec3 localPos;
in vec3 worldPos;
out vec4 outColor;

void main(void)
{
	vec3 du = dFdx( worldPos );
	vec3 dv = dFdy( worldPos );
	//estimating that 1 unit in world space is about a cm in the real world (since the face has a bounding sphere radius of 15)
	float stretchU = .005 / length( du );
	float stretchV = .005 / length( dv );
	//outputs 1 / dist in world space that each pixel covers (in meters/10000 to get values between 0-1)
	outColor = vec4(stretchU, stretchV, 0, 1); // A two-component texture
}