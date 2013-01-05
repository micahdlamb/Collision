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
in float gAlpha[];
out vec3 fColor;
out float fAlpha;

out vec2 TexCoord;

void main()
{
	float gBillboardSize = 0.04f * worldTransform[0][0];

	fColor = gColor[0];
	fAlpha = gAlpha[0];

    vec3 pos = gl_in[0].gl_Position.xyz;
    vec3 toCamera = normalize(eyePos - pos);
    vec3 right = normalize(cross(vec3(0,1,0),toCamera));
	vec3 up = normalize(cross(toCamera,right));

	right *= gBillboardSize;
	up *= gBillboardSize;

    gl_Position = eye * vec4(pos, 1.0);
    TexCoord = vec2(0.0, 0.0);
    EmitVertex();

	pos += right;
    gl_Position = eye * vec4(pos, 1.0);
    TexCoord = vec2(1.0, 0.0);
    EmitVertex();

	pos -= right;
    pos += up;
    gl_Position = eye * vec4(pos, 1.0);
    TexCoord = vec2(0.0, 1.0);
    EmitVertex();

    pos += right;
    gl_Position = eye * vec4(pos, 1.0);
    TexCoord = vec2(1.0, 1.0);
    EmitVertex();

    EndPrimitive();
}
