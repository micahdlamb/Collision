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
int iterations=4;
int slices=30;
float extrude=.5f;

bool curveClosed(){
	switch(curveType){
		case CLOSED_QUADRATIC_B_SPLINE:
		case CLOSED_CUBIC_B_SPLINE:
			return true;
	}
	return false;
}