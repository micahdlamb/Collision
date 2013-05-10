#version 410 core

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
	float time;
	float deltaTime;
	vec2 screenDim;
};

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 gWorldPos[];
in vec3 gLocalPos[];

out vec3 fWorldPos;
out vec3 fLocalPos;
out float fZ;
out vec4 shadowCoords[6];

void main()
{
	/*does nothing for now
	vec3 A = gWorldPos[1] - gWorldPos[0];
    vec3 B = gWorldPos[2] - gWorldPos[0];
    
    fNormal = normalize(cross(A, B));
    */
	fWorldPos = gWorldPos[0];
    fLocalPos = gLocalPos[0];
	fZ = gl_in[0].gl_Position.z / gl_in[0].gl_Position.w;
	shadowCoords[0] = lights[0].eye * vec4(fWorldPos,1);
    gl_Position = gl_in[0].gl_Position; EmitVertex();

	fWorldPos = gWorldPos[1];
    fLocalPos = gLocalPos[1];
	fZ = gl_in[1].gl_Position.z / gl_in[1].gl_Position.w;
	shadowCoords[0] = lights[0].eye * vec4(fWorldPos,1);
    gl_Position = gl_in[1].gl_Position; EmitVertex();

	fWorldPos = gWorldPos[2];
    fLocalPos = gLocalPos[2];
	fZ = gl_in[2].gl_Position.z / gl_in[2].gl_Position.w;
	shadowCoords[0] = lights[0].eye * vec4(fWorldPos,1);
    gl_Position = gl_in[2].gl_Position; EmitVertex();

    EndPrimitive();
}
