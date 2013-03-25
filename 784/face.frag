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
	mat4 normalTransform;
};

uniform sampler2D normals;
uniform sampler2D color;
uniform sampler2D diffuse;
uniform samplerCube reflections;
uniform int reflectionsOn;

in vec3 localPos;
in vec3 worldPos;
in vec2 uv;
//in vec2 uv0;
//in vec2 uv1;

out vec4 outColor;

void main(void)
{
	/*
	vec2 uv = vec2(
		( fwidth( uv0.x ) < fwidth( uv1.x )-0.001 )? uv0.x : uv1.x
		,( fwidth( uv0.y ) < fwidth( uv1.y )-0.001 )? uv0.y : uv1.y
	);
	*/

	//hardcoded for now
	float amb = .2;
	float dif = texture(diffuse, uv * screenDim / vec2(2048)).r;
	float spec = .4;
	float shininess = 30;
	vec3 norm = texture(normals, uv).rgb*2-vec3(1);
	norm = normalize(vec3(normalTransform * vec4(norm,0)));//should change normalTransform to be mat3 but I'd have to change it all over :(
	if (!gl_FrontFacing) norm *= -1;

	//Color the surface
	vec3 diffuseColor = texture(color, uv).rgb;
	vec3 lightColor = lights[0].color;
	vec3 lightPos = lights[0].pos;
	vec3 lightDir = normalize(lightPos - worldPos);
	vec3 eyeDir = normalize(eyePos - worldPos);
	vec3 reflectDir = reflect(-lightDir, norm);

	//ambient
	vec3 color = amb * diffuseColor;
	//diffuse
	color += diffuseColor * dif * max(0.0, dot(norm, lightDir));
	//specular
	color += lightColor * spec * pow(max(0.0, dot(reflectDir, eyeDir)), shininess);
	
	if (reflectionsOn == 1)
		color = mix(color, texture(reflections, reflect(-eyeDir, norm)).rgb, .2);

	outColor = vec4(color,1);
}