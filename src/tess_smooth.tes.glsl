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
};

layout(std140) uniform Object {
	mat4 worldTransform;
	mat4 normalTransform;
};

layout(quads, equal_spacing, ccw) in;

struct OutPatch {
	vec2 texCoords[4];
	vec3 worldPositions[16];
};

// attributes of the output CPs
in patch OutPatch p;

out vec3 fWorldPos;
out vec3 fLocalPos;
out vec3 fNormal;

uniform sampler2D heightMap;
uniform sampler2D normalMap;

vec2 interpolate2(vec2 bl, vec2 br, vec2 tr, vec2 tl){
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

	vec2 b = mix(bl,br,u);
	vec2 t = mix(tl,tr,u);
	return mix(b,t,v);
} 

vec3 interpolate3(vec3 bl, vec3 br, vec3 tr, vec3 tl){
	float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

	vec3 b = mix(bl,br,u);
	vec3 t = mix(tl,tr,u);
	return mix(b,t,v);
} 

vec3 bezier16(){
	float u = gl_TessCoord.x, v = gl_TessCoord.y;

	mat4 B = mat4(
		-1,3,-3,1
		,3,-6,3,0
		,-3,3,0,0
		,1,0,0,0);

	mat4 BT = transpose(B);

	mat4 Px = mat4(
		p.worldPositions[0].x, p.worldPositions[1].x, p.worldPositions[2].x, p.worldPositions[3].x, 
		p.worldPositions[4].x, p.worldPositions[5].x, p.worldPositions[6].x, p.worldPositions[7].x, 
		p.worldPositions[8].x, p.worldPositions[9].x, p.worldPositions[10].x, p.worldPositions[11].x, 
		p.worldPositions[12].x, p.worldPositions[13].x, p.worldPositions[14].x, p.worldPositions[15].x );

	mat4 Py = mat4(
		p.worldPositions[0].y, p.worldPositions[1].y, p.worldPositions[2].y, p.worldPositions[3].y, 
		p.worldPositions[4].y, p.worldPositions[5].y, p.worldPositions[6].y, p.worldPositions[7].y, 
		p.worldPositions[8].y, p.worldPositions[9].y, p.worldPositions[10].y, p.worldPositions[11].y, 
		p.worldPositions[12].y, p.worldPositions[13].y, p.worldPositions[14].y, p.worldPositions[15].y );

	mat4 Pz = mat4(
		p.worldPositions[0].z, p.worldPositions[1].z, p.worldPositions[2].z, p.worldPositions[3].z, 
		p.worldPositions[4].z, p.worldPositions[5].z, p.worldPositions[6].z, p.worldPositions[7].z, 
		p.worldPositions[8].z, p.worldPositions[9].z, p.worldPositions[10].z, p.worldPositions[11].z, 
		p.worldPositions[12].z, p.worldPositions[13].z, p.worldPositions[14].z, p.worldPositions[15].z );

	mat4 cx = B * Px * BT;
	mat4 cy = B * Py * BT;
	mat4 cz = B * Pz * BT;

	vec4 U = vec4(u*u*u, u*u, u, 1);
	vec4 V = vec4(v*v*v, v*v, v, 1);

	float x = dot(cx * V, U);
	float y = dot(cy * V, U);
	float z = dot(cz * V, U);
	return vec3(x, y, z);
}


void main()
{
    // Interpolate the attributes of the output vertex using the barycentric coordinates
	vec2 texCoords = interpolate2(p.texCoords[0], p.texCoords[1], p.texCoords[2], p.texCoords[3]);
	float y = texture(heightMap, texCoords.yx).x;
	fLocalPos = vec3(texCoords.x,y,texCoords.y);
    fWorldPos = bezier16();
	fNormal = normalize((normalTransform * vec4(texture(normalMap, texCoords.yx).xyz,0)).xyz);
	gl_Position = eye * vec4(fWorldPos, 1.0);
}

