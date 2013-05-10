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
		vector<vec3> r;
		mat4 delta = rotate(mat4(1), 360.f/slices, vec3(0,1,0));
		mat4 spin;
		for (int i=0; i < slices; i++, spin *= delta)
			for (size_t j=0; j < curve.size(); j++)
				r.push_back(vec3(spin * vec4(curve[j],0,1)));

		return r;
	}

	vector<vec3> extrusion(vector<vec2> curve){
		vector<vec3> r;
		for (int i=0; i < 3; i++)
			for (size_t j=0; j < curve.size(); j++)
				r.push_back((vec3(curve[j],(-1+i)*extrude)).swizzle(Z,X,Y));

		return r;
	}

	vector<vec3> sweep(vector<vec2> curve1, vector<vec2> curve2){
		#define FORWARD(i) vec3(i==0 ? curve2[i+1]-curve2[i] : (i==curve2.size()-1 ? curve2[i] - curve2[i-1] : curve2[i+1]-curve2[i-1]),0)
		
		vector<vec3> r;
		r.resize(curve2.size()*curve1.size());
		#pragma omp parallel for
		for (int i=0; i < (int)curve2.size(); i++){
			vec3 forward = FORWARD(i)//vec3(i != curve2.size()-1 ? curve2[i+1]-curve2[i] : curve2[i] - curve2[i-1], 0)
				,x = normalize(perp(vec3(0,1,0), forward));
			if (dot(vec3(1,0,0), forward) < 0)//found through guess and check
				x *= -1;
			vec3 y = normalize(cross(forward, x));

			for (size_t j=0; j < curve1.size(); j++)
				r[i*curve1.size() + j] = vec3(curve2[i],0) + x * extrude * curve1[j].x + y * extrude * curve1[j].y;
		}

		return r;
	}

};