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

struct Intersection {
	bool hit;
	float dist;
	vec3 pt;
	vec3 normal;
};

struct ClosestIntersection {
	int sphere;
	Intersection intersection;
};

//or could use inout instead
struct QuadraticResult {
	bool real;
	float s1;
	float s2;
};

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

	//mininum value above 0
	i.dist = qr.s1 < 0 ? qr.s2 : (qr.s2 < 0 ? qr.s1 : min(qr.s1, qr.s2));
	
	i.pt = r.start + r.direction * i.dist;
	i.normal = normalize(i.pt - s.center);

	return i;
}

float EPS = .01;
int numSpheres;
Sphere spheres[25];

vec3 lightColor;
vec3 lightPos;

vec3 scaleColor(vec3 color){
	float m = max(max(color.x,color.y),color.z);
	return m > 1 ? color / m : color;
}

vec3 shade(Ray ray, ClosestIntersection ci, out Ray reflection);

vec4 trace(Ray ray){
	float ref = .5;
	float contribution = 1;
	vec3 color = vec3(0);
	bool miss = false;
	Ray reflection;
	ClosestIntersection ci;
	for (int i=0; i < 5 && !miss; i++){
		ci.sphere = -1;
		ci.intersection.dist = 999999;

		for (int j=0; j < numSpheres; j++){
			Intersection x = intersect(ray, spheres[j]);
			if (x.hit && x.dist < ci.intersection.dist){
				ci.sphere = j;
				ci.intersection = x;
			}
		}

		if (ci.sphere != -1){
			color += shade(ray, ci, reflection) * contribution;
			contribution *= ref;
		}
		else{
			//if (i==0)
				//return vec4(1,0,0,.5);
			color += textureLod(reflections, ray.direction, 0).rgb * contribution;

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
		ray = reflection;
	}
	return vec4(scaleColor(color),1);
}

vec3 shade(Ray ray, ClosestIntersection ci, out Ray reflection){
	float amb = 0;
	float dif = .5;
	float spec = 1;
	float shininess = 30;

	Sphere s = spheres[ci.sphere];
	Intersection i = ci.intersection;

	vec3 lightDir = normalize(lightPos - i.pt);
	//vec3 eyeDir = normalize(eyePos - i.pt);
	vec3 reflectDir = reflect(-lightDir, i.normal);

	//ambient
	vec3 color = s.color * amb;
	//diffuse
	color += lightColor * s.color * dif * max(0.0, dot(i.normal, lightDir));
	//specular
	color += lightColor * spec * pow(max(0.0, dot(reflectDir, -ray.direction)), shininess);

	reflection.direction = reflect(ray.direction, i.normal);
	reflection.start = i.pt + EPS * reflection.direction;
	//color += trace(r).rgb * ref;

	return vec3(color);
}

void main(void)
{
	lightColor = lights[0].color;
	lightPos = lights[0].pos;

	numSpheres = 3;
	spheres[0].color = vec3(0,.4,1);
	spheres[0].center = vec3(-2,-2,-5);
	spheres[0].radius = 2;
	
	spheres[1].color = vec3(0,1,.4);
	spheres[1].center = vec3(4,0,-9);
	spheres[1].radius = 2;

	spheres[2].color = vec3(1,.4,0);
	spheres[2].center = vec3(0,8,-13);
	spheres[2].radius = 2;

	Ray r;
	r.start = eyePos;
	r.direction = normalize(worldPos - eyePos);

	outColor = trace(r);
}