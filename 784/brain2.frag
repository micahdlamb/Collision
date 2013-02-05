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

struct AABB {
    vec3 Min;
    vec3 Max;
};

//find intersection points of a ray with a box
bool intersectBox(Ray r, AABB aabb, out float t0, out float t1)
{
    vec3 invR = 1.0 / r.direction;
    vec3 tbot = invR * (aabb.Min-r.start);
    vec3 ttop = invR * (aabb.Max-r.start);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);
    return t0 <= t1;
}

//for dithering
float EPS = .001;
//step size of ray through cube, smaller values cause large slowdown
uniform float delta = .005;
//control how fast color is absorbed
uniform float absorbtion = 10;
//only absorb color when density change > this value
uniform float threshold = .01;
uniform sampler3D densities;
uniform sampler3D normals;

in vec3 localPos;
flat in vec3 localEyePos;
flat in vec3 localLightPos;
out vec4 outColor;

//custom coloring function based on density
vec3 getColor(float density){
	return mix(vec3(0,1,0), vec3(.29,0,.51), density*20);
}

void main(void)
{
	//construct ray to step through cube
	Ray r;
	r.start = localEyePos;
	r.direction = normalize(localPos - localEyePos);

	//figure out where ray from eye hit front of cube
	float tnear, tfar;
	intersectBox(r, AABB(vec3(-.5),vec3(.5)), tnear, tfar);
	//if eye is in cube then start ray at eye
	if (tnear < 0.0) tnear = 0.0;
	//+ vec3(.5) for local -> texture space
	r.start += r.direction * tnear + vec3(.5);
	
	//accumulate color and alpha
	vec3 color = vec3(0);
	float alpha = 0;

	float forward = delta;
	
	//dither
	float dither = fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453) * forward + EPS;
	vec3 mover = r.start + r.direction * dither;
	
	//while ray still in cube
	while (alpha < 1 && mover.x >= 0 && mover.x <= 1 && mover.y >= 0 && mover.y <= 1 && mover.z >= 0 && mover.z <= 1){
		float density = texture(densities, mover).r;
		if (density > threshold){
			vec3 norm = texture(normals,mover).rgb;
			float mag = length(norm);

			if (mag > 0){
				norm /= mag;
				float a = mag * absorbtion;

				/*
				if (isnan(norm.x) || isnan(norm.y) || isnan(norm.z)){
					color = vec3(0,1,0);
					alpha = 1;
					break;
				}
				*/

				//outColor = vec4(normalize(norm) - norm, 1);
				//return;

				float amb = .2;
				float dif = .8;
				float spec = 1;
				float shininess = 30;

				vec3 pos = mover-vec3(.5);
				vec3 diffuseColor = getColor(density);
				vec3 lightColor = lights[0].color;

				vec3 lightDir = normalize(localLightPos - pos);
				vec3 eyeDir = normalize(localEyePos - pos);
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
		}
		mover += r.direction * forward;
	}

	alpha = min(alpha,1);
	outColor = vec4(color,alpha);
}