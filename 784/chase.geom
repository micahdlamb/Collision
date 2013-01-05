#version 330

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

layout(std140) uniform Object {
	mat4 worldTransform;
	mat4 normalTransform;
};

layout(points) in;
layout(triangle_strip) out;
layout(max_vertices = 4) out;

in vec3 gColor[];
out vec3 fColor;

out vec2 coord;
out float radius;
out vec3 center;
out vec3 toEye;
out vec3 right;
out vec3 up;

void main()
{
	radius = 0.04f * 25 * worldTransform[0][0];

	fColor = gColor[0];

    center = gl_in[0].gl_Position.xyz;
    toEye = normalize(eyePos - center);
    right = normalize(cross(vec3(0,1,0),toEye));
	up = normalize(cross(toEye,right));

	vec3 r = right * radius * 2;
	vec3 u = up * radius * 2;

	vec3 pos = center;
	pos -= .5*r + .5*u;
    gl_Position = eye * vec4(pos, 1.0);
    coord = vec2(0.0, 0.0);
    EmitVertex();

	pos += r;
    gl_Position = eye * vec4(pos, 1.0);
    coord = vec2(1.0, 0.0);
    EmitVertex();

	pos -= r;
    pos += u;
    gl_Position = eye * vec4(pos, 1.0);
    coord = vec2(0.0, 1.0);
    EmitVertex();

    pos += r;
    gl_Position = eye * vec4(pos, 1.0);
    coord = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
