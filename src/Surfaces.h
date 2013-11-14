#pragma once

extern int slices;
extern float extrude;

namespace Surfaces {
	/*
	vec3 perp(vec3 v){
		vec3 up(0,1,0);
		float dp = dot(v, up);
		if (dp == 0) up = vec3(1,0,0);
		//else if (dp < 0) up = -up;
		return cross(v, up);
	}
	*/
	vector<vec3> revolution(vector<vec2> curve){
		vector<vec3> pts;
		mat4 delta = rotate(mat4(1), 360.f/slices, vec3(0,1,0));
		mat4 spin;
		for (int i=0; i < slices; i++, spin *= delta)
			for (size_t j=0; j < curve.size(); j++)
				pts.push_back(vec3(spin * vec4(curve[j],0,1)));

		return pts;
	}

	vector<vec3> extrusion(vector<vec2> curve){
		vector<vec3> pts;
		for (int i=0; i < 3; i++)
			for (size_t j=0; j < curve.size(); j++)
				pts.push_back((vec3(curve[j],(-1+i)*extrude)).swizzle(Z,X,Y));

		return pts;
	}

	//some template conditional way to do this?
	inline vec3 toVec3(vec3 v){return v;}
	inline vec3 toVec3(vec2 v){return vec3(v.x,0, -v.y);}
	
	template<class V>
	vector<vec3> sweep(vector<vec2> around, vector<V> along, float scale=1){
		#define FORWARD(i) toVec3(i==0 ? along[i+1]-along[i] : (i==along.size()-1 ? along[i] - along[i-1] : along[i+1]-along[i-1]))
		
		vector<vec3> r;
		r.resize(along.size()*around.size());
		#pragma omp parallel for
		for (int i=0; i < (int)along.size(); i++){
			vec3 forward = FORWARD(i)//vec3(i != along.size()-1 ? along[i+1]-along[i] : along[i] - along[i-1], 0)
				,x = normalize(perp(vec3(0,1,0), forward));
			//if (dot(vec3(1,0,0), forward) < 0)//found through guess and check
				//x *= -1;
			vec3 y = normalize(cross(forward, x));

			for (size_t j=0; j < around.size(); j++){
				vec3 center = toVec3(along[i]);//use template conditional when I can figure it out
				r[i*around.size() + j] = center + x * scale * around[j].x + y * scale * around[j].y;
			}
		}

		return r;
	}

};