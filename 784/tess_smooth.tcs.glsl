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
};

layout(std140) uniform Object {
	mat4 worldTransform;
	mat4 normalTransform;
};

// define the number of CPs in the output patch
layout (vertices = 1) out;

uniform float lod;
uniform vec2 screenDim;
uniform sampler2D heightMap;
//uniform sampler2D normalMap;

// attributes of the input CPs
in vec3 tcWorldPos[];
in vec2 tcTexCoords[];

struct OutPatch {
	vec2 texCoords[4];
	//vec3 normals[16];
	vec3 worldPositions[16];
};

// attributes of the output CPs
out patch OutPatch p;

vec3 ndc(vec3 world){
	vec4 v = eye * vec4(world,1);
	v /= v.w;
	return v.xyz;
}

//vertex should be ndc
bool offScreen(vec3 vertex){
	//z gets big fast as shit
	return !(abs(vertex.z+1) < 1.93) && any(lessThan(vertex, vec3(-1.5)) || greaterThan(vertex, vec3(1.5)));
}

//vertex should be ndc
vec2 screen(vec3 vertex){
	return (clamp(vertex.xy,-1,1) + 1) * .5 * screenDim;
}

float level(float d){
	return clamp(lod * d, 1, 64);
}

void main()
{
	// Set the control points of the output patch
	for (int i=0; i < 4; i++)
		p.texCoords[i] = tcTexCoords[i];
	
	for (int i=0; i < 16; i++){
		vec2 coord = vec2(mix(tcTexCoords[0].x,tcTexCoords[1].x,(i%4)/3.f)
			,mix(tcTexCoords[0].y,tcTexCoords[2].y,(i/4)/3.f));
		float y = texture(heightMap, coord.yx).x;
		p.worldPositions[i] = (worldTransform * vec4(coord.x,y,coord.y,1)).xyz;
	}

	if (ID == 0){
		
		vec3 ndc0 = ndc(tcWorldPos[0]);
		vec3 ndc1 = ndc(tcWorldPos[1]);
		vec3 ndc2 = ndc(tcWorldPos[2]);
		vec3 ndc3 = ndc(tcWorldPos[3]);

		if (offScreen(ndc0)
			&& offScreen(ndc1)
			&& offScreen(ndc2)
			&& offScreen(ndc3)){

			gl_TessLevelOuter[0] = 0;
			gl_TessLevelOuter[1] = 0;
			gl_TessLevelOuter[2] = 0;
			gl_TessLevelOuter[3] = 0;

			gl_TessLevelInner[0] = 0;
			gl_TessLevelInner[1] = 0;
			
			return;
		}



		// Calculate the distance from the camera to the 4 control pts
		/*
		float d0 = distance(eyePos, tcWorldPos[0]);
		float d1 = distance(eyePos, tcWorldPos[1]);
		float d2 = distance(eyePos, tcWorldPos[2]);
		float d3 = distance(eyePos, tcWorldPos[3]);

		gl_TessLevelOuter[0] = level(mix(d3,d0,.5));
		gl_TessLevelOuter[1] = level(mix(d0,d1,.5));
		gl_TessLevelOuter[2] = level(mix(d1,d2,.5));
		gl_TessLevelOuter[3] = level(mix(d2,d3,.5));

		gl_TessLevelInner[0] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[3], .5);
		gl_TessLevelInner[1] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[2], .5);
		*/

		vec2 s0 = screen(ndc0);
		vec2 s1 = screen(ndc1);
		vec2 s2 = screen(ndc2);
		vec2 s3 = screen(ndc3);

		gl_TessLevelOuter[0] = level(distance(s3,s0));
		gl_TessLevelOuter[1] = level(distance(s0,s1));
		gl_TessLevelOuter[2] = level(distance(s1,s2));
		gl_TessLevelOuter[3] = level(distance(s2,s3));

		//not sure which one is better
		gl_TessLevelInner[0] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[3], .5);
		gl_TessLevelInner[1] = mix(gl_TessLevelOuter[0], gl_TessLevelOuter[2], .5);
		//gl_TessLevelInner[0] = mix(gl_TessLevelOuter[3], gl_TessLevelOuter[0], .5);
		//gl_TessLevelInner[1] = mix(gl_TessLevelOuter[1], gl_TessLevelOuter[2], .5);

	}
} 