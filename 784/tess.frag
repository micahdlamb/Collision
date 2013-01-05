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

in vec3 fWorldPos;
in vec3 fLocalPos;

uniform sampler2D normalMap;

uniform sampler2D noise;
uniform sampler2D water;
uniform sampler2D grass;
uniform sampler2D stone;
uniform sampler2D rock;
uniform sampler2D snow;
uniform sampler2D pebbles;

//uniform samplerCube skyBox;
uniform sampler2D skyBox;

//shadows
uniform sampler2D shadowMap;
in vec4 shadowCoords[6];

float chebyshevUpperBound(float distance, vec2 moments)
{
	// Surface is fully lit. as the current fragment is before the light occluder
	if (distance <= moments.x)
		return 1.0 ;

	// The fragment is either in shadow or penumbra. We now use chebyshev's upperBound to check
	// How likely this pixel is to be lit (p_max)
	float variance = moments.y - (moments.x*moments.x);
	variance = max(variance,0.00002);

	float d = distance - moments.x;
	float p_max = variance / (variance + d*d);

	return p_max;
}

out vec4 outColor;

void main(void)
{
	//hardcoded light and color for now
	float amb = .2;
	float dif = .8;
	float spec = .1;
	float shininess = 30;
	int blur = 0;

	vec3 diffuseColor;
	vec3 norm = normalize((normalTransform * vec4(texture(normalMap, fLocalPos.zx).xyz,0)).xyz);
	if (!gl_FrontFacing) norm *= -1;

	vec3 pos = fLocalPos;

	//make these the same in tcs
	float numWaves = 100;
	float waterSpeed = time*.005;
	float normalDeflection = 2.25;

	float n1 = texture(noise,pos.xz).x
		,height = pos.y+(n1-.5)*.6
		,slope = 1 - max(0,dot(norm,vec3(0,1,0)))
		,waterScale = 5
		,grassScale = 65
		,stoneScale = 20
		,rockScale = 50
		,snowScale = 50
		,pebblesScale = 65;

	vec2 waterCoords = (pos.xz + vec2(0,waterSpeed)) * waterScale
		,grassCoords = pos.xz * grassScale
		,stoneCoords = pos.xz * stoneScale
		,snowCoords = pos.xz * snowScale
		,rockCoords = pos.xz * rockScale
		,pebblesCoords = pos.xz * pebblesScale;

	vec2 waterGradX = dFdx(waterCoords), waterGradY = dFdy(waterCoords)
		,grassGradX = dFdx(grassCoords), grassGradY = dFdy(grassCoords)
		,stoneGradX = dFdx(stoneCoords), stoneGradY = dFdy(stoneCoords)
		,snowGradX = dFdx(snowCoords), snowGradY = dFdy(snowCoords)
		,rockGradX = dFdx(rockCoords), rockGradY = dFdy(rockCoords)
		,pebblesGradX = dFdx(pebblesCoords), pebblesGradY = dFdy(pebblesCoords);

	bool isWater = false;

	if (pos.y <= .015){
		isWater = true;

		diffuseColor = textureGrad(water, waterCoords, waterGradX, waterGradY).xyz;
		norm = normalize(vec3(0,1,0) + vec3(0,0,(normalDeflection/45)*sin((pos.z + waterSpeed) * 3.14 * numWaves)));
		spec = 2;
	}
	else if (height < .2)
		diffuseColor = textureGrad(grass, grassCoords, grassGradX, grassGradY).xyz;
	else if (height < .25)
		diffuseColor = mix(textureGrad(grass, grassCoords, grassGradX, grassGradY).xyz, textureGrad(stone, stoneCoords, stoneGradX, stoneGradY).xyz, (height - .2) / .05);
	else if (height < .6)
		diffuseColor = textureGrad(stone, stoneCoords, stoneGradX, stoneGradY).xyz;
	else if (height < .65){
		float r = (height - .6) / .05;
		spec = r*.5;
		diffuseColor = mix(textureGrad(stone, stoneCoords, stoneGradX, stoneGradY).xyz, textureGrad(snow, snowCoords, snowGradX, snowGradY).xyz, r);
	}
	else{
		spec = .5;
		diffuseColor = textureGrad(snow, snowCoords, snowGradX, snowGradY).xyz;
	}
	
	//mix in rock texture for high slopes
	if (slope > .6)
		diffuseColor = mix(diffuseColor, textureGrad(rock, rockCoords, rockGradX, rockGradY).xyz, pow((slope-.6)*(1/.4),.5));

	vec3 lightColor = lights[0].color;
	vec3 lightPos = lights[0].pos;
	vec3 lightDir = normalize(lightPos - fWorldPos);
	vec3 eyeDir = normalize(eyePos - fWorldPos);
	float dist = distance(eyePos,fWorldPos);
	vec3 reflectDir = reflect(-lightDir, norm);

	//shadow
	vec3 shadowCoords0 = (shadowCoords[0] / shadowCoords[0].w).xyz;
	shadowCoords0 = shadowCoords0*.5 + .5;
	vec2 moments0 = texture(shadowMap, shadowCoords0.xy).rg;
	float shadowedIntensity = pow(chebyshevUpperBound(shadowCoords0.z, moments0),4);

	//outColor = vec4(shadowCoords0.z,shadowCoords0.z,shadowCoords0.z,1);
	//return;

	//ambient (give a little directional light)
	vec3 color = mix(diffuseColor, diffuseColor * max(0, dot(norm, vec3(0,1,0))), .5) * amb;
	//vec3 color = diffuseColor * amb;
	//diffuse
	color += shadowedIntensity * diffuseColor * dif * max(0.0, dot(norm, lightDir));
	//specular
	color += shadowedIntensity * lightColor * spec * pow(max(0.0, dot(reflectDir, eyeDir)), shininess);
	
	//water reflection & ground
	if (isWater){
		vec2 c = gl_FragCoord.xy / vec2(2048,2048);
		float distort = .05*(1/dist);
		c *= (1-distort) + distort*sin((pos.z + waterSpeed) * 3.14 * numWaves);
		//color = mix(color, textureLod(skyBox, reflect(-eyeDir, norm), blur).rgb, .75);
		vec3 reflection = texture(skyBox, c).rgb;
		reflection = mix(color, reflection, .75);
		//vec2 refractCoords = .1*refract(-eyeDir, norm, .8).xz;
		float h = pos.y/.015;
		vec3 waterBedColor = mix(textureGrad(pebbles, pebblesCoords, pebblesGradX, pebblesGradY).rgb, textureGrad(grass, grassCoords, grassGradX, grassGradY).rgb, pow(h,.2));
		waterBedColor = mix(color, waterBedColor, .8 + .2*h);
		
		color = mix(reflection, shadowedIntensity * waterBedColor, .5*eyeDir.y);
	}

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