enum CurveTypes {
	BEZIER=0
	,QUADRATIC_B_SPLINE
	,CLOSED_QUADRATIC_B_SPLINE
	,CUBIC_B_SPLINE
	,CLOSED_CUBIC_B_SPLINE
};

enum SurfaceTypes {
	REVOLUTION=0
	,EXTRUSION
	,SWEEP
};

int curveType=0;
int surfaceType=0;
int iterations=0;
int slices=3;
float extrude=.5f;

bool curveClosed(){
	switch(curveType){
		case CLOSED_QUADRATIC_B_SPLINE:
		case CLOSED_CUBIC_B_SPLINE:
			return true;
	}
	return false;
}