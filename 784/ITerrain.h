#pragma once

struct ITerrain {
	virtual float getY(float x, float z)=0;
	virtual vec3 getNormal(float x, float z)=0;
};