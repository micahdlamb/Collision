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

struct Ray {
	vec3 start;
	vec3 direction;
};

float EPS = .001;

vec3 getColor(float depth){
	vec3 colorz[3] = vec3[3](
		vec3(1,0,0)
		,vec3(0,1,0)
		,vec3(0,0,1)
	);
	return colorz[int(depth * 3.00001)];
}

uniform float delta;
uniform float absorbtion;
uniform sampler3D volume;

in vec3 localPos;
flat in vec3 localEyePos;
flat in vec3 localLightPos;
out vec4 outColor;

void main(void)
{
	Ray r;
	r.start = localPos + vec3(.5);//.5 to convert to 0-1
	r.direction = normalize(localPos - localEyePos);
	//float delta = .005;
	vec3 color = vec3(0);
	float alpha = 0;

	vec3 mover = r.start;
	vec3 forward = r.direction * delta;
	mover += forward * EPS;//for rounding errors
	//float random = fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453);
	//mover += forward * random;//dither

	while (alpha < 1 && mover.x >= 0 && mover.x <= 1 && mover.y >= 0 && mover.y <= 1 && mover.z >= 0 && mover.z <= 1){
		float density = texture(volume, mover).r;
		float a = 1;//density * delta * absorbtion;
		if (density > .02){
			vec3 gradient = vec3(
				texture(volume, mover+vec3(1,0,0)*delta).r - texture(volume, mover+vec3(-1,0,0)*delta).r
				,texture(volume, mover+vec3(0,1,0)*delta).r - texture(volume, mover+vec3(0,-1,0)*delta).r
				,texture(volume, mover+vec3(0,0,1)*delta).r - texture(volume, mover+vec3(0,0,-1)*delta).r);
			vec3 norm = normalize(-gradient);

			float amb = .2;
			float dif = .8;
			float spec = 1;
			float shininess = 30;

			vec3 pos = mover-vec3(.5);
			vec3 diffuseColor = getColor(density*15);
			vec3 lightColor = lights[0].color;
			vec3 lightPos = localLightPos;

			vec3 lightDir = normalize(lightPos - pos);
			vec3 eyeDir = normalize(eyePos - pos);
			vec3 reflectDir = reflect(-lightDir, norm);

			//ambient
			vec3 c = diffuseColor * amb;
			//diffuse
			c += diffuseColor * dif * max(0.0, dot(norm, lightDir));
			//specular
			c += lightColor * spec * pow(max(0.0, dot(reflectDir, eyeDir)), shininess);

			color += c * a * (1-alpha);
			alpha += a;
		}

		mover += forward;
	}

	alpha = min(alpha,1);
	outColor = vec4(color,alpha);
}