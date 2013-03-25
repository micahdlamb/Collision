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

in vec3 localPos;
in vec3 worldPos;
in vec3 normal;
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

float EPS = .01;

vec3 lightColor;
vec3 lightPos;

vec3 scaleColor(vec3 color){
	float m = max(max(color.x,color.y),color.z);
	return m > 1 ? color / m : color;
}

vec3 shade(Ray ray, Intersection c, out Ray next);

vec4 trace(Ray ray){
	float ref = .5;
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
	//float ri = rIndex + rand(i.pt);
	//next.direction = refract(ray.direction, i.normal, i.inside ? ri : 1/ri);
	next.direction = reflect(ray.direction, i.normal);
	next.start = i.pt + EPS  * i.normal;
	//next.start = i.pt + EPS * next.direction;

	return vec3(color);
}


struct Lens {
	vec3 center;
	float radius;
	float ri;
	float aperture;
	int front;
};

layout(std140) uniform Lenses {
	int numLenses;
	Lens lenses[10];
};

struct LensIntersection {
	vec3 pos;
	vec3 normal;
};

bool intersectLens(Ray r, Lens lens, out LensIntersection hit){
	//plane
	if (lens.radius == 0){
		vec3 normal = vec3(0,0,1);
		float d = lens.center.z;
		float t = -(dot(r.start, normal) + d) / dot(r.direction, normal);
		hit.pos = r.start + t * r.direction;
		if (length(vec2(hit.pos.x, hit.pos.y)) > (lens.aperture / 2))
			return false;
		hit.normal = normal;
		return true;
	
	//sphere
	} else {
		
		float a = dot(r.direction, r.direction)
			,b = dot(r.direction*2, r.start - lens.center)
			,c = dot(r.start - lens.center, r.start - lens.center) - lens.radius * lens.radius;
	
		QuadraticResult qr = solveQuadratic(a,b,c);
		if (!qr.real || (qr.s1 < 0 && qr.s2 < 0))
			return false;
		
		//float t = lens.front==1 ? min(qr.s1, qr.s2) : max(qr.s1, qr.s2);
		//hit.pos = r.start + t * r.direction;
		

		vec3 pos1 = r.start + qr.s1 * r.direction;
		vec3 pos2 = r.start + qr.s2 * r.direction;
		if (lens.front==1)
			hit.pos = pos1.z < pos2.z ? pos2 : pos1;
		else
			hit.pos = pos1.z < pos2.z ? pos1 : pos2;

			
		if (lens.front==1 && hit.pos.z < lens.center.z)
			return false;
		if (lens.front==0 && hit.pos.z > lens.center.z)
			return false;


		if (length(vec2(hit.pos.x, hit.pos.y)) > (lens.aperture / 2))
			return false;
		hit.normal = normalize(hit.pos - lens.center);
		//if (lens.front == 0)
			//hit.normal *= -1;
		if (dot(hit.normal, r.direction) > 0)
			hit.normal *= -1;

		return true;
	}
}

int traceRayThroughLenses(Ray rin, out Ray rout){
	float ri = 1;//start in air
	LensIntersection hit;
	for (int i=0; i < numLenses; i++){
		if (!intersectLens(rin, lenses[i], hit)){
			rout = rin;
			//return i;
			ri = lenses[i].ri;
			continue;
		}

		float toRi = lenses[i].ri;
		rin.start = hit.pos;
		//outColor = vec4(lenses[i].center.z);
		rin.direction = refract(rin.direction, hit.normal, ri / toRi);
		ri = toRi;
	}
	rout = rin;
	return -1;
}

vec3 getRandPtOnFirstLens(){
	float dim = lenses[0].aperture;
	float x,y;
	x = fract(sin(gl_FragCoord.x * 12.9898 + gl_FragCoord.y * 78.233) * 43758.5453) * dim - .5*dim;
	y = fract(sin(gl_FragCoord.x * 321.328 + gl_FragCoord.y * 32.213) * 41234.5453) * dim - .5*dim;
	float z = lenses[0].center.z;
	/*
	vec3 center = lenses[0].center;
	float radius = lenses[0].radius;
	float x2 = pow(x-center.x, 2.f);
	float y2 = pow(y-center.y, 2.f);
	float a = 1;
	float b = -2*center.z;
	float c = center.z*center.z + x2 + y2 - radius*radius;
	QuadraticResult qr = solveQuadratic(a,b,c);
	float z = max(qr.s1,qr.s2);
	*/
	return vec3(x,y,z);
}

void main(void)
{
	aabb = AABB(vec3(-25,0,-25), vec3(25,50,25));
	lightColor = lights[0].color;
	lightPos = lights[0].pos;

	Ray r;
	//r.start = eyePos;
	r.start = worldPos;
	r.direction = normalize(worldPos - eyePos);
	//r.direction = normalize(vec3(0,0,-1));
	//r.direction = normalize(getRandPtOnFirstLens() - worldPos);

	//outColor = trace(r) * lenses[6].front;
	vec3 colors[12];
	colors[0] = vec3(0,0,0);
	colors[1] = vec3(0,0,1);
	colors[2] = vec3(0,1,0);
	colors[3] = vec3(0,1,1);
	colors[4] = vec3(1,0,0);
	colors[5] = vec3(1,0,1);
	colors[6] = vec3(1,1,0);
	colors[7] = vec3(1,1,1);
	colors[8] = vec3(1,0,0);
	colors[9] = vec3(0,0,1);
	colors[10] = vec3(0,1,0);
	colors[11] = vec3(0,1,1);
	Ray rout;
	int throughLens = traceRayThroughLenses(r, rout);

	//if (throughLens != -1)
		//outColor = vec4(colors[throughLens], 1);
	//else
		outColor = trace(rout);
}