#version 410 core
#define ID gl_InvocationID

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

// define the number of CPs in the output patch
layout (vertices = 4) out;

uniform float lod=.1;

// attributes of the input CPs
in vec3 tcWorldPos[];
in vec2 tcTexCoords[];

// attributes of the output CPs
out vec2 teTexCoords[];

vec3 ndc(vec3 world){
	vec4 v = eye * vec4(world,1);
	v /= v.w;
	return v.xyz;
}

//vertex should be ndc
bool offScreen(vec3 vertex){
	float z = vertex.z * .5 + .5;
	
	float w = 1 + pow(1-z,1) * 100;
	return vertex.z < -1 || vertex.z > 1 || any(lessThan(vertex.xy, vec2(-w)) || greaterThan(vertex.xy, vec2(w)));
}

bool visible(vec3 v1, vec3 v2, vec3 v3, vec3 v4){
	if (   v1.x < -1 && v2.x < -1 && v3.x < -1 && v4.x < -1
		|| v1.x > 1 && v2.x > 1 && v3.x > 1 && v4.x > 1
		|| v1.y < -1 && v2.y < -1 && v3.y < -1 && v4.y < -1
		|| v1.y > 1 && v2.y > 1 && v3.y > 1 && v4.y > 1
		|| v1.z < -1 && v2.z < -1 && v3.z < -1 && v4.z < -1
		|| v1.z > 1 && v2.z > 1 && v3.z > 1 && v4.z > 1)
		return false;
	return true;
}

//vertex should be ndc
vec2 screen(vec3 vertex){
	return (clamp(vertex.xy,-1,1) + 1) * .5 * screenDim;
}

float level(float d){
	//return clamp(lod * d, 1, 64);//screen projection approach
	return clamp(lod * 2000/d, 1, 64);//dist from eye approach
}

void main()
{
	// Set the control points of the output patch
	teTexCoords[ID] = tcTexCoords[ID];
	
	if (ID == 0){
		
		vec3 ndc0 = ndc(tcWorldPos[0]);
		vec3 ndc1 = ndc(tcWorldPos[1]);
		vec3 ndc2 = ndc(tcWorldPos[2]);
		vec3 ndc3 = ndc(tcWorldPos[3]);
		
		if (offScreen(ndc0)
			&& offScreen(ndc1)
			&& offScreen(ndc2)
			&& offScreen(ndc3)){
		//if (!visible(ndc0,ndc1,ndc2,ndc3)){

			gl_TessLevelOuter[0] = 0;
			gl_TessLevelOuter[1] = 0;
			gl_TessLevelOuter[2] = 0;
			gl_TessLevelOuter[3] = 0;

			gl_TessLevelInner[0] = 0;
			gl_TessLevelInner[1] = 0;
			
			return;
		}



		//Calc tessellation based on distance from eye
		float d0 = distance(eyePos, tcWorldPos[0]);
		float d1 = distance(eyePos, tcWorldPos[1]);
		float d2 = distance(eyePos, tcWorldPos[2]);
		float d3 = distance(eyePos, tcWorldPos[3]);

		/*increase polygons if more change?
		float c0 = dot(tcWorldPos[3],tcWorldPos[0]);
		float c1 = dot(tcWorldPos[0],tcWorldPos[1]);
		float c2 = dot(tcWorldPos[1],tcWorldPos[2]);
		float c3 = dot(tcWorldPos[2],tcWorldPos[3]);
		*/

		gl_TessLevelOuter[0] = level(mix(d3,d0,.5));
		gl_TessLevelOuter[1] = level(mix(d0,d1,.5));
		gl_TessLevelOuter[2] = level(mix(d1,d2,.5));
		gl_TessLevelOuter[3] = level(mix(d2,d3,.5));

		gl_TessLevelInner[0] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[3], .5);
		gl_TessLevelInner[1] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[2], .5);
		

		//Calc tessellation based on distance between vertices projected onto screen
		/*
		vec2 s0 = screen(ndc0);
		vec2 s1 = screen(ndc1);
		vec2 s2 = screen(ndc2);
		vec2 s3 = screen(ndc3);

		gl_TessLevelOuter[0] = level(distance(s3,s0));
		gl_TessLevelOuter[1] = level(distance(s0,s1));
		gl_TessLevelOuter[2] = level(distance(s1,s2));
		gl_TessLevelOuter[3] = level(distance(s2,s3));
		*/

		//not sure which one is better
		//gl_TessLevelInner[0] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[3], .5);
		//gl_TessLevelInner[1] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[2], .5);
		//gl_TessLevelInner[0] = mix(gl_TessLevelOuter[3], gl_TessLevelOuter[0], .5);
		//gl_TessLevelInner[1] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[2], .5);
		float l = max(max(gl_TessLevelOuter[0],gl_TessLevelOuter[1]),max(gl_TessLevelOuter[2],gl_TessLevelOuter[3]));
		gl_TessLevelInner[0] = l;
		gl_TessLevelInner[1] = l;
	}
} 