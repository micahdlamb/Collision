#pragma once

//temp fix to get around dependency loop (fuck c++)
void computeSurface();

struct Pt {
	Pt(){}
	Pt(vec2 pos, vec3 color):pos(pos),color(color){}
	vec2 pos;
	vec3 color;
};

struct Surface;

struct ControlCurve : public Viewport {

	static const int MAXCONTROLPTS = 100;
	static const int MAXCURVEPTS = 100000;//bugginess will occur when this gets overflowed
	static const int ptSize = 12, lineThickness = 3, curveThickness = 6;

	GVector<Pt> controlPts;
	GVector<Pt> curvePts;
	
	GVector<Pt> origin;

	Shader shader;

	//colors
	const vec3 backgroundColor
	,lineColor
	,hoverColor
	,selectedColor
	,selectedHoverColor
	,curveColor;

	bool colorz;

	int hoverPt, draggingPt, selectedPt;

	ControlCurve(float x, float y, float w, float h, vec3 backgroundColor):
		hoverPt(-1),draggingPt(-1),selectedPt(-1)
		,Viewport(x,y,w,h)
		,controlPts(GL_LINE_STRIP, GL_DYNAMIC_DRAW)
		,curvePts(GL_LINE_STRIP, GL_DYNAMIC_DRAW)
		,origin(GL_LINES)
		,backgroundColor(backgroundColor)
		,lineColor(.6,.1,1)
		,hoverColor(0,1.0f,0.0f)
		,selectedColor(.8f,.8f,0)
		,selectedHoverColor(1.0f, .5f, 0)
		,curveColor(0, 0, 1)
		,colorz(false)
	{

	}

	void operator()(){
		//should be updated every draw but I'll assume it isn't changed
		glPointSize((GLfloat)ptSize);
		glEnable(GL_POINT_SMOOTH);
		glLineStipple(14, 0xAAAA);
		
		shader("line.vert", "line.frag");

		//I don't think there is anyway for these two vao's to be shared since they point to dif buffers
		controlPts.bind();
		controlPts.buffer(NULL, MAXCONTROLPTS * sizeof(Pt));
		controlPts.in(2,GL_FLOAT,sizeof(Pt),0);
		controlPts.in(3,GL_FLOAT,sizeof(Pt),offsetof(Pt,color));
		controlPts.unbind();

		curvePts.bind();
		curvePts.buffer(NULL, MAXCURVEPTS * sizeof(Pt));
		curvePts.in(2,GL_FLOAT,sizeof(Pt),0);
		curvePts.in(3,GL_FLOAT,sizeof(Pt),offsetof(Pt,color));
		curvePts.unbind();

		//init origin
		origin.bind();
		origin.buffer(NULL, MAXCURVEPTS * sizeof(Pt));
		origin.in(2,GL_FLOAT,sizeof(Pt),0);
		origin.in(3,GL_FLOAT,sizeof(Pt),offsetof(Pt,color));
		origin.unbind();

		origin.push_back(Pt(vec2(0,-.5),vec3(1,0,1)));
		origin.push_back(Pt(vec2(0,.5),vec3(1,0,1)));
		origin.push_back(Pt(vec2(-.5,0),vec3(1,.5,0)));
		origin.push_back(Pt(vec2(.5,0),vec3(1,.5,0)));

		//throw a border in here
		origin.push_back(Pt(vec2(-.999,-.999),vec3(0,0,0)));
		origin.push_back(Pt(vec2(.999, -.999),vec3(0,0,0)));
		origin.push_back(Pt(vec2(.999, -.999),vec3(0,0,0)));
		origin.push_back(Pt(vec2(.999, .999),vec3(0,0,0)));
		origin.push_back(Pt(vec2(.999, .999),vec3(0,0,0)));
		origin.push_back(Pt(vec2(-.999, .999),vec3(0,0,0)));
		origin.push_back(Pt(vec2(-.999, .999),vec3(0,0,0)));
		origin.push_back(Pt(vec2(-.999, -.999),vec3(0,0,0)));

	}


	vector<vec2> getControlPtPositions(){
		return getPositions(controlPts);
	}

	vector<vec2> getCurvePtPositions(){
		return getPositions(curvePts);
	}

	//anyway to template a field extraction function?
	vector<vec2> getPositions(vector<Pt>& pts){
		vector<vec2> r;
		for (size_t i=0; i < pts.size(); i++)
			r.push_back(pts[i].pos);
		return r;
	}


	void setCurvePtPositions(vector<vec2>& pts){
		curvePts.resize(pts.size());
		for (size_t i=0; i < pts.size(); i++){
			curvePts[i].pos = pts[i];
			curvePts[i].color = colorz ? rainbowColors() : curveColor;//meh never changes as of now
		}
		curvePts.pushAll();
	}

	void clear(){
		hoverPt=-1;
		draggingPt=-1;
		selectedPt=-1;
		controlPts.clear();
		curvePts.clear();
	}

	void compute(){
		setCurvePtPositions(build<vec2>(getControlPtPositions()));
		computeSurface();
	}

	template <class T>
	vector<T> build(vector<T>& pts){
		if (pts.size() < 2) return vector<T>();

		switch (curveType){
			case BEZIER:
				return Bezier::create<T>(pts);
			case QUADRATIC_B_SPLINE:
				return BSpline::quadratic_chaikin<T>(pts);
			case CLOSED_QUADRATIC_B_SPLINE:
				return BSpline::quadratic_chaikin<T>(pts, true);
			case CUBIC_B_SPLINE:
				return BSpline::cubic<T>(pts);
			default://CLOSED_CUBIC_B_SPLINE:
				return BSpline::cubic<T>(pts, true);
		}
	}



	void redraw(bool recompute=false){
		if (recompute)
			compute();

		//using idle draw loop to redraw for now
	}

	virtual void draw(){
		glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, 1);
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		shader.enable();

		glEnable(GL_LINE_STIPPLE);

		glLineWidth((GLfloat)lineThickness/2.f);
		origin.drawAll();
		glLineWidth((GLfloat)lineThickness);
		controlPts.drawAll();
		glDisable(GL_LINE_STIPPLE);
		controlPts.drawAll(GL_POINTS);

		glLineWidth((GLfloat)curveThickness);
		curvePts.drawAll();
		printGLErrors("display");
	}

	void colorPt(int index){
		vec3 color;
		if (index == selectedPt && index == hoverPt)
			color = selectedHoverColor;
		else if (index == selectedPt)
			color = selectedColor;
		else if (index == hoverPt)
			color = hoverColor;
		else
			color = lineColor;
		controlPts[index].color = color;
		controlPts.push(index);
	}

	int onControlPt(int x, int y){
		float radius = ptSize/2.f;//approximate
		for (size_t i=0; i < controlPts.size(); i++){
			if (length(vec2(x,y) - screen(controlPts[i].pos)) < radius)
				return (int)i;
		}
		return -1;
	}

	vec2 screen(vec2 pos){
		return (pos * .5f + vec2(.5)) * vec2(w,h);
	}

	vec2 ndc(int x, int y){
		return (vec2(x,y)/vec2(w,h))*2.0f - vec2(1);
	}

	//called by the ViewportManager
	virtual void mouseButton(int button, int state, int x, int y){
		switch (button){
			case GLUT_LEFT_BUTTON:
				switch (state){
					case GLUT_DOWN:
					{
						vec2 pos = ndc(x,y);
						//cout << "click at: " << pos.x << "," << pos.y << endl;
					
						if (hoverPt == -1){
							Pt pt(pos,hoverColor);
							if (selectedPt == -1){
								controlPts.push_back(pt);
								hoverPt = controlPts.size()-1;
							} else {
								controlPts.insertBefore(selectedPt, pt);
								hoverPt = selectedPt;
								selectedPt++;
							}
							redraw(true);
						}

						draggingPt = hoverPt;

						break;
					}
					case GLUT_UP:
						draggingPt = -1;
						break;
				}
		}
	}


	virtual void mouseMove(int x, int y) {
		if (draggingPt != -1){
			controlPts[draggingPt].pos = ndc(x,y);
			controlPts.push(draggingPt);
			redraw(true);
		}
	}

	virtual void passiveMouseMove(int x, int y){
		int p = onControlPt(x, y);
		if (p == hoverPt)
			return;

		//unhover pt
		if (hoverPt != -1){
			int prev = hoverPt;
			hoverPt = -1;
			colorPt(prev);
			redraw();
		}
		//hover new pt
		if (p != -1){
			hoverPt = p;
			colorPt(hoverPt);
			redraw();
		}
	}

	virtual void keyDown (unsigned char key, int x, int y) {

		switch (key) {
			case 127:
				if (hoverPt != -1){
					controlPts.erase(hoverPt);
					if (selectedPt){
						if (hoverPt == selectedPt)
							selectedPt = -1;
						else if (hoverPt < selectedPt)
							selectedPt--;
					}
					hoverPt = -1;
					draggingPt = -1;
					redraw(true);
				}
				break;
			case 's':
				if (hoverPt != -1){
					int prev = selectedPt;
					selectedPt = selectedPt != hoverPt ? hoverPt : -1;
					colorPt(hoverPt);
					if (prev != -1)
						colorPt(prev);
					redraw();
				}
				break;
		}


	}

	virtual void keyUp(unsigned char key, int x, int y){



	}
};
