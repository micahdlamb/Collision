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

uniform samplerCube background;

in vec3 worldPos;
out vec4 outColor;

struct Sphere {
	vec3 color;
	vec3 center;
	float radius;
};

vec3 lightColor;
vec3 lightPos;
float EPS = .01;
int numSpheres;
Sphere spheres[25];

struct Ray {
	vec3 start;
	vec3 direction;
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

bool solveQuadratic(out float s1, out float s2, float a, float b, float c){
	float e = b*b - (a*c)*4;
	if (e < 0)
		return false;//imaginary solution
		
	float discriminant = sqrt(e);
	s1 = (-b - discriminant)/(2*a);
	s2 = (-b + discriminant)/(2*a);
	return true;
}

Intersection intersect(Ray r, Sphere s){
	Intersection i;
	i.hit = false;

	float a = dot(r.direction, r.direction)
		,b = dot(r.direction*2, r.start - s.center)
		,c = dot(r.start - s.center, r.start - s.center) - s.radius * s.radius;
	
	float t1, t2;
	bool hit = solveQuadratic(t1,t2,a,b,c);
	if (!hit || (t1 < 0 && t2 < 0))
		return i;

	i.hit = true;

	//mininum value above 0
	i.dist = t1 < 0 ? t2 : (t2 < 0 ? t1 : min(t1, t2));
	
	i.pt = r.start + r.direction * i.dist;
	i.normal = normalize(i.pt - s.center);

	return i;
}

vec3 scaleColor(vec3 color){
	float m = max(max(color.x,color.y),color.z);
	return m > 1 ? color / m : color;
}

bool find(out ClosestIntersection ci, Ray ray){
	ci.sphere = -1;
	ci.intersection.dist = 999999;

	for (int j=0; j < numSpheres; j++){
		Intersection x = intersect(ray, spheres[j]);
		if (x.hit && x.dist < ci.intersection.dist){
			ci.sphere = j;
			ci.intersection = x;
		}
	}
	return ci.sphere != -1;
}

vec3 shade(Ray ray, ClosestIntersection ci, out Ray next){
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

	next.direction = reflect(ray.direction, i.normal);
	next.start = i.pt + EPS * next.direction;
	//color += trace(r).rgb * ref;

	return vec3(color);
}

vec3 trace(Ray ray){
	//not sure if this method of summing up color is correct
	float strength = .5;
	float alpha = 0;

	vec3 color = vec3(0);
	Ray next;
	ClosestIntersection ci;
	for (int i=0; i < 5; i++){
		if (find(ci, ray)){
			color += shade(ray, ci, next) * strength * (1-alpha);
			alpha += strength * (1-alpha);
		}
		else {
			vec3 bg = texture(background, ray.direction).rgb;
			
			//show light in background
			float dist = length(lightPos - ray.start);
			float intensity = pow(max(0.0, dot(normalize(lightPos - ray.start), normalize(ray.direction))),dist*dist);
			if (intensity > .8) intensity = 1;
			if (intensity > .01)
				bg = mix(bg, lightColor, intensity);
			color += bg * (1-alpha);
			break;
		}

		ray = next;
	}
	return scaleColor(color);
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

	outColor = vec4(trace(r), 1);
}