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
};

layout(std140) uniform Object {
	mat4 worldTransform;
	mat4 normalTransform;
};

in vec3 fWorldPos;
in vec3 fLocalPos;
in vec3 fNormal;

uniform sampler2D normalMap;

uniform sampler2D noise;
uniform sampler2D water;
uniform sampler2D grass;
uniform sampler2D stone;
uniform sampler2D rock;
uniform sampler2D snow;

out vec4 outColor;

void main(void)
{
	//hardcoded light and color for now
	float amb = .1;
	float dif = .8;
	float spec = .1;
	float shininess = 30;


	vec3 diffuseColor;
	vec3 norm = normalize(fNormal);
	if (!gl_FrontFacing) norm *= -1;

	vec3 pos = fLocalPos;

	/*float slopeAngle = acos(dot(norm,vec3(0,1,0)));
	if (slopeAngle > radians(90.0f))
		slopeAngle = radians(180.0f) - slopeAngle;
	*/
	float slope = max(0,dot(norm,vec3(0,1,0)));//the correct way refuses to work
	
	float n = (texture(noise,pos.xz).x - .5)*.5
		,height = pos.y+n
		,waterScale = 5
		,grassScale = 20
		,stoneScale = 20
		,rockScale = 50
		,snowScale = 10;

	if (pos.y <= .01){
		diffuseColor = vec3(texture(water,(pos.xz+vec2(0,time*.005)) * waterScale));
		spec = 1;
	}
	else if (height < .2)
		diffuseColor = vec3(texture(grass, pos.xz * grassScale));
	else if (height < .25)
		diffuseColor = mix(texture(grass, pos.xz * grassScale).xyz, texture(stone, pos.xz * stoneScale).xyz, (height - .2) / .05);
	else if (height < .65)
		diffuseColor = vec3(texture(stone, pos.xz * stoneScale));
	else if (height < .7)
		diffuseColor = mix(texture(stone, pos.xz * stoneScale).xyz, texture(snow, pos.xz * snowScale).xyz, (height - .65) / .05);
	else
		diffuseColor = vec3(texture(snow, pos.xz * snowScale));
	
	//mix in rock texture for high slopes
	if (slope < .5){
		if (height < .65)
			diffuseColor = mix(texture(rock, pos.xz * rockScale).xyz, diffuseColor, pow(slope*2,6.0f));
		else if (height < .7){
			vec3 x = mix(texture(rock, pos.xz * rockScale).xyz, diffuseColor, pow(slope*2,6.0f));
			diffuseColor = mix(x, diffuseColor, (height - .65) / .05);
		}
	}

	vec3 lightColor = lights[0].color;
	vec3 lightPos = lights[0].pos;
	vec3 lightDir = normalize(lightPos - fWorldPos);
	vec3 eyeDir = normalize(eyePos - fWorldPos);
	vec3 reflectDir = reflect(-lightDir, norm);

	//ambient
	vec3 color = diffuseColor * amb;
	//diffuse
	color += diffuseColor * dif * max(0.0, dot(norm, lightDir));
	//specular
	color += lightColor * spec * pow(max(0.0, dot(reflectDir, eyeDir)), shininess);
	

	//experimental: show light
	if (length(lightPos - eyePos) < length(fWorldPos - eyePos)){
		float dist = length(lightPos - eyePos);
		float intensity = pow(max(0.0, dot(normalize(lightPos - eyePos), normalize(fWorldPos - eyePos))),dist*dist);
		if (intensity > .8) intensity = 1;
		if (intensity > .01)
			color = mix(color, lightColor, intensity);
	}
	
	outColor = vec4(color,1);
}