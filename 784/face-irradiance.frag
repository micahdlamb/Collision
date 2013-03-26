#version 330 core

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
	mat3 normalTransform;
};

float chebyshevUpperBound(float distance, vec2 moments)
{
	// Surface is fully lit. as the current fragment is before the light occluder
	if (distance <= moments.x)
		return 1.0 ;

	// The fragment is either in shadow or penumbra. We now use chebyshev's upperBound to check
	// How likely this pixel is to be lit (p_max)
	float variance = moments.y - (moments.x*moments.x);
	variance = max(variance,0.0000002);

	float d = distance - moments.x;
	float p_max = variance / (variance + d*d);

	return p_max;
}

uniform sampler2D normals;
uniform sampler2D shadowMap;

in vec3 localPos;
in vec3 worldPos;
in vec2 uv;
in vec3 shadowCoords[6];

out vec4 outColor;

void main(void)
{
	vec2 moments0 = texture(shadowMap, shadowCoords[0].xy).rg;
	float shadowedIntensity = chebyshevUpperBound(shadowCoords[0].z, moments0);
	
	//hardcoded for now
	vec3 norm = texture(normals, uv).rgb*2-vec3(1);
	norm = normalize(normalTransform * norm);
	if (!gl_FrontFacing) norm *= -1;

	vec3 lightPos = lights[0].pos;
	vec3 lightDir = normalize(lightPos - worldPos);

	//diffuse
	outColor = vec4(vec3(1) * max(0.0, dot(norm, lightDir)) * shadowedIntensity, 1);
}