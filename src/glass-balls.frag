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

uniform samplerCube reflections;
uniform sampler2D perlin;
uniform float rIndex;
uniform float noise;
uniform float alpha;
uniform float timeNoise;

in vec3 worldPos;
out vec4 outColor;

struct Ray {
	vec3 start;
	vec3 direction;
};

struct Sphere {
	vec3 color;
	vec3 center;
	float radius;
};

struct AABB {
    vec3 Min;
    vec3 Max;
};

layout(std140) uniform Spheres {
	int numSpheres;
	Sphere spheres[10];
};


struct Intersection {
	bool hit;
	float dist;
	vec3 pt;
	vec3 normal;
	vec3 color;
	bool inside;
};

//or could use inout instead
struct QuadraticResult {
	bool real;
	float s1;
	float s2;
};

float rand(vec3 v){
	float tn = sin(time) * timeNoise;
	return texture(perlin,v.xy + v.yz).x * noise + tn;
	//return fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453 * time)-.5;
	/*
	return vec3(
		fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453 * time)
		,fract(sin(gl_FragCoord.x * 17.1717 + gl_FragCoord.y * 73.23233) * 432158.79453 * time)
		,fract(sin(gl_FragCoord.x * 33.3333 + gl_FragCoord.y * 69.23323) * 434218.44723 * time)
	);
	*/
}

QuadraticResult solveQuadratic(float a, float b, float c){
	QuadraticResult qr;
	qr.real = false;

	float e = b*b - (a*c)*4;
	if (e < 0)
		return qr;//imaginary solution
	qr.real = true;
	float d = sqrt(e);
	qr.s1 = (-b - d)/(2*a);
	qr.s2 = (-b + d)/(2*a);
	return qr;
}

AABB aabb;
float centerY = 25;
Intersection intersectBox(Ray r){
	Intersection i;
	i.hit = false;

	vec3 invR = 1.0 / r.direction;
	vec3 tbot = invR * (aabb.Min-r.start);
	vec3 ttop = invR * (aabb.Max-r.start);
	vec3 tmin = min(ttop, tbot);
	vec3 tmax = max(ttop, tbot);
	vec2 t = max(tmin.xx, tmin.yz);
	float t0 = max(t.x, t.y);
	t = min(tmax.xx, tmax.yz);
	float t1 = min(t.x, t.y);
	if (t0 < 0) return i;

	i.hit = true;

	if (t0 < 0){
		i.dist = t1;
		i.inside = true;
	} else {
		i.dist = t0;
		i.inside = false;
	}

	i.pt = r.start + r.direction * i.dist;
	if (abs(i.pt.x) > abs(i.pt.y-centerY) && abs(i.pt.x) > abs(i.pt.z))
		i.normal = vec3(i.pt.x>0?1:-1,0,0);
	else if (abs(i.pt.y-centerY) > abs(i.pt.x) && abs(i.pt.y-centerY) > abs(i.pt.z))
		i.normal = vec3(0,i.pt.y-centerY>0?1:-1,0);
	else
		i.normal = vec3(0,0,i.pt.z>0?1:-1);
	if (i.inside)
		i.normal *= -1;
	i.color = vec3(0,0,1);
	return i;
}

Intersection intersect(Ray r, Sphere s){
	Intersection i;
	i.hit = false;

	float a = dot(r.direction, r.direction)
		,b = dot(r.direction*2, r.start - s.center)
		,c = dot(r.start - s.center, r.start - s.center) - s.radius * s.radius;
	
	QuadraticResult qr = solveQuadratic(a,b,c);
	if (!qr.real || (qr.s1 < 0 && qr.s2 < 0))
		return i;

	i.hit = true;

	float s1 = min(qr.s1, qr.s2);
	float s2 = max(qr.s1, qr.s2);

	if (s1 > 0){
		i.inside = false;
		i.dist = s1;
	} else {
		i.inside = true;
		i.dist = s2;
	}
		
	i.pt = r.start + r.direction * i.dist;
	i.normal = normalize(i.pt - s.center);
	if (i.inside)
		i.normal *= -1;
	i.color = s.color;
	return i;
}

float EPS = .001;

vec3 lightColor;
vec3 lightPos;

vec3 scaleColor(vec3 color){
	float m = max(max(color.x,color.y),color.z);
	return m > 1 ? color / m : color;
}

vec3 shade(Ray ray, Intersection c, out Ray next);

vec4 trace(Ray ray){
	float ref = .8;
	float contribution = 1;
	vec3 color = vec3(0);
	bool miss = false;
	Ray next;
	Intersection ci;
	for (int i=0; i < 8 && !miss; i++){
		ci.dist = 6969;
		ci.hit = false;
		for (int j=0; j < numSpheres; j++){
			Intersection x = intersect(ray, spheres[j]);
			if (x.hit && x.dist < ci.dist)
				ci = x;
		}
		/*
		Intersection x = intersectBox(ray);
		if (x.hit && x.dist < ci.dist)
			ci = x;
		*/
		if (ci.hit){
			color = mix(color, shade(ray, ci, next), contribution);
			contribution *= ref;
		}
		else {
			//if (i==0)
				//return vec4(1,0,0,.5);
			//color += textureLod(reflections, ray.direction, 0).rgb * contribution;
			color = mix(color, textureLod(reflections, ray.direction, 0).rgb, contribution);
			miss = true;
		}

		//show light in background
		if (i==0){
			float dist = length(lightPos - ray.start);
			float intensity = pow(max(0.0, dot(normalize(lightPos - ray.start), normalize(ray.direction))),dist*dist);
			if (intensity > .8) intensity = 1;
			if (intensity > .01)
				color = mix(color, lightColor, intensity);
		}
		ray = next;
	}
	return vec4(scaleColor(color),alpha);
}

vec3 shade(Ray ray, Intersection i, out Ray next){
	float amb = 0;
	float dif = .8;
	float spec = 1;
	float shininess = 30;

	vec3 lightDir = normalize(lightPos - i.pt);
	//vec3 eyeDir = normalize(eyePos - i.pt);
	vec3 reflectDir = reflect(-lightDir, i.normal);

	//ambient
	vec3 color = i.color * amb;
	//diffuse
	color += lightColor * i.color * dif * max(0.0, dot(i.normal, lightDir));
	//specular
	if (!i.inside)
		color += lightColor * spec * pow(max(0.0, dot(reflectDir, -ray.direction)), shininess);

	//vec3 n = noise * rand();
	float ri = rIndex + rand(i.pt);
	next.direction = refract(ray.direction, i.normal, i.inside ? ri : 1/ri);
	next.start = i.pt + EPS * -1 * i.normal;
	//next.start = i.pt + EPS * next.direction;

	return vec3(color);
}

void main(void)
{
	aabb = AABB(vec3(-25,0,-25), vec3(25,50,25));
	lightColor = lights[0].color;
	lightPos = lights[0].pos;

	/*
	numSpheres = 3;
	spheres[0].color = vec3(0,.4,1);
	spheres[0].center = vec3(-2,-2,-5);
	spheres[0].radius = 3;
	//spheres[0].alpha = 1;
	
	spheres[1].color = vec3(0,1,.4);
	spheres[1].center = vec3(4,0,-9);
	spheres[1].radius = 3;
	//spheres[0].alpha = .5;

	spheres[2].color = vec3(1,.4,0);
	spheres[2].center = vec3(0,8,-13);
	spheres[2].radius = 3;
	//spheres[0].alpha = .1;
	*/

	Ray r;
	r.start = eyePos;
	r.direction = normalize(worldPos - eyePos);

	outColor = trace(r);
}