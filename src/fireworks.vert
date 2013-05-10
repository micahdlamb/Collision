#version 330

layout (location = 0) in float Type;
layout (location = 1) in vec3 Position;
layout (location = 2) in vec3 Velocity;
layout (location = 3) in float Age;
layout (location = 4) in float Color;

layout(std140) uniform Object {
	mat4 worldTransform;
	mat4 normalTransform;
};

out vec3 gColor;
out float gAlpha;

void main()
{

	if (Color == 0)
		gColor = vec3(1,0,0);
	else if (Color == 1)
		gColor = vec3(0,1,0);
	else if (Color == 2)
		gColor = vec3(0,0,1);
	else if (Color == 3)
		gColor = vec3(1,1,0);
	else if (Color == 4)
		gColor = vec3(1,0,1);
	else if (Color == 5)
		gColor = vec3(.4,1,0);
	else if (Color == 6)
		gColor = vec3(0,0,0);
	else if (Color == 7)
		gColor = vec3(1,.5,0);
	else
		gColor = normalize(Velocity) * .5 + vec3(.5);


	if (Type == 2.0f)
		gAlpha = pow(1 - Age / 6000,4);
	else
		gAlpha = 1;

	gl_Position = worldTransform * vec4(Position, 1.0);
    //gl_Position = eye * worldTransform * vec4(Position, 1.0);
}

/*
layout (location = 0) in vec3 Position;

void main()
{
    gl_Position = vec4(Position, 1.0);
}
*/