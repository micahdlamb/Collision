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

layout(std140) uniform Object {
	mat4 worldTransform;
	mat4 normalTransform;
};

layout(quads, equal_spacing, ccw) in;

in vec2 teTexCoords[];

out vec3 gWorldPos;
out vec3 gLocalPos;

uniform float waveAmp=.0035;

uniform sampler2D heightMap;

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

void main()
{
	// Interpolate the attributes of the output vertex using the barycentric coordinates
	vec2 texCoords = interpolate2(teTexCoords[0], teTexCoords[1], teTexCoords[2], teTexCoords[3]);

	float y = texture(heightMap, texCoords.yx).x;
	gLocalPos = vec3(texCoords.x,y,texCoords.y);

	vec3 localPos = gLocalPos;
	localPos.y = max(localPos.y,.01);
	//make water wave
	if (y < .015){
		float waterSpeed = time*.005;
		float numWaves = 100;
		localPos.y += waveAmp*sin((localPos.z + waterSpeed) * 3.14 * numWaves + (3.14/4));
	}

	gWorldPos = (worldTransform * vec4(localPos, 1)).xyz;
	gl_Position = eye * vec4(gWorldPos, 1.0);
}

