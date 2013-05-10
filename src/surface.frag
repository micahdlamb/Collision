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
};

uniform int pattern;

uniform samplerCube reflections;
uniform int reflectionsOn;
uniform int blur;

in vec3 localPos;
in vec3 worldPos;
in vec3 normal;
out vec4 outColor;

void main(void)
{
	//hardcoded light and color for now
	float amb = .2;
	float dif = .8;
	float spec = 1;
	float shininess = 30;
	vec3 norm = normalize(normal);
	if (!gl_FrontFacing) norm *= -1;

	//Color the surface
	vec3 diffuseColor;
	if (pattern == 0)
		diffuseColor = vec3(.5,.2,.7);
	else if (pattern == 1)//colorz
		diffuseColor = vec3(1 - abs(2*(localPos.x - floor(localPos.x)) - 1),1 - abs(2*(localPos.y - floor(localPos.y)) - 1),1 - abs(2*(localPos.z - floor(localPos.z)) - 1));
	else if (pattern == 2){//stripes
		
		vec3 color1 = vec3(.4,.5,.4);
		vec3 color2 = vec3(.42,.26,.15);
		float stripes = 10.f;
		diffuseColor = int((localPos.y+1000) * stripes) % 2 == 0 ? color1 : color2;
		//lightColor = vec3(0,0,1);
	}
	else if (pattern == 3)//colorz world pos
		diffuseColor = vec3(1 - abs(2*(worldPos.x - floor(worldPos.x)) - 1),1 - abs(2*(worldPos.y - floor(worldPos.y)) - 1),1 - abs(2*(worldPos.z - floor(worldPos.z)) - 1));
	else if (pattern == 4){
		diffuseColor = vec3(1) * pow(localPos.y*5+.05,4);
		spec = .4;
	}

	vec3 lightColor = lights[0].color;
	vec3 lightPos = lights[0].pos;
	vec3 lightDir = normalize(lightPos - worldPos);
	vec3 eyeDir = normalize(eyePos - worldPos);
	vec3 reflectDir = reflect(-lightDir, norm);

	//ambient
	vec3 color = diffuseColor * amb;
	//diffuse
	color += diffuseColor * dif * max(0.0, dot(norm, lightDir));
	//specular
	color += lightColor * spec * pow(max(0.0, dot(reflectDir, eyeDir)), shininess);
	

	if (reflectionsOn == 1)
		color = mix(color, textureLod(reflections, reflect(-eyeDir, norm), blur).rgb, .5);

	//experimental: show light
	if (length(lightPos - eyePos) < length(worldPos - eyePos)){
		float dist = length(lightPos - eyePos);
		float intensity = pow(max(0.0, dot(normalize(lightPos - eyePos), normalize(worldPos - eyePos))),dist*dist);
		if (intensity > .8) intensity = 1;
		if (intensity > .01)
			color = mix(color, lightColor, intensity);
	}

	outColor = vec4(color,1);
}