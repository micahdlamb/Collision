#include "MyWindow.h"

const char* title = "Ray Tracer";

struct RayTracer : public Viewport, public Scene, public FPInput {
	Shader shader;

	UniformSampler background;
	UniformMat4 transform;
	mat4 placeSquare;

	CubeMap* backgrounds[3];

	float nearPlane, farPlane, fov;

	RayTracer(float x, float y, float w, float h):
		Viewport(x,y,w,h)
		,FPInput(4)
		,nearPlane(.1f), farPlane(1001.f), fov(60.f)
		,placeSquare(1)
	{
		Scene::operator()(this);
		shader("rayTracer.vert", "rayTracer.frag");
		transform("transform", &shader);

		Scene::globals.lights[0].color = vec3(1,1,1);
		Scene::globals.lights[0].pos = vec3(0,0,25);

		//put square .5 units in front of viewer and scale by 100
		placeSquare[3] = vec4(0,0,-.5,1);
		placeSquare[0][0] = 100;
		placeSquare[1][1] = 100;
		placeSquare[2][2] = 100;

		glDisable(GL_DEPTH_TEST);

		#define STR(x) #x
		#define CM(file) STR(cubemaps/clouds/##file)
		char* clouds[] = {CM(px.png),CM(nx.png),CM(py.png),CM(ny.png),CM(pz.png),CM(nz.png)};
		backgrounds[0] = new CubeMap(clouds, IL_ORIGIN_UPPER_LEFT);
		
		#define CM(file) STR(cubemaps/deadmeat/##file)
		char* deadmeat[] = {CM(px.jpg),CM(nx.jpg),CM(py.jpg),CM(ny.jpg),CM(pz.jpg),CM(nz.jpg)};
		backgrounds[1] = new CubeMap(deadmeat, IL_ORIGIN_UPPER_LEFT);

		#define CM(file) STR(cubemaps/hills/##file)
		char* hills[] = {CM(px.png),CM(nx.png),CM(py.png),CM(ny.png),CM(pz.png),CM(nz.png)};
		backgrounds[2] = new CubeMap(hills, IL_ORIGIN_UPPER_LEFT);
		#undef CM

		background("background",&shader,backgrounds[2]);
	}

	void setBackground(int i){
		shader.enable();
		background = backgrounds[i];
	}

	void Scene::draw(){
		shader.enable();
		transform = inverse(Viewer::matrix()) * placeSquare;//put square in front of viewer
		Shapes::square()->draw();
	}

	void Viewport::draw(){
		frame();
		FPInput::frame();
		Scene::frame();

		((Scene*)this)->draw();

		Clock::printFps(250);
	}
	
	void frame(){
		//put key handler code here
		if (keys['c'])
			Scene::globals.lights[0].color = rainbowColors(.01);//change the light color when held pressed

	}

	//called by the ViewportManager
	virtual void resize(){
		Scene::resize(perspective(fov, (GLfloat)w/h, nearPlane, farPlane),w,h);
	}
	
	virtual void mouseButton(int button, int state, int x, int y){
		FPInput::mouseButton(button, state, x, y);
	}

	virtual void mouseMove(int x, int y) {
		FPInput::mouseMove(x, y);
	}

	virtual void passiveMouseMove(int x, int y){

	}

	virtual void keyDown (unsigned char key, int x, int y) {
		FPInput::keyDown(key, x, y);
		
		switch (key){
			case '1':
				setBackground(0);
				break;
			case '2':
				setBackground(1);
				break;
			case '3':
				setBackground(2);
				break;
		}
	}

	virtual void keyUp(unsigned char key, int x, int y){
		FPInput::keyUp(key, x, y);
	}

};


MyWindow win;
RayTracer* rt;

void init(void)
{
	win();
	rt = new RayTracer(0,0,1,1);
	win.add(rt);
}

void init_glui(){

}

void display()
{
	glClearColor(.9f,.8f,.7f,1);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	win.draw();
	printGLErrors("/display");
}

void idle(){
	glutPostRedisplay();
}

void reshape(int w, int h){
	win.resize(w,h);
	glutPostRedisplay();
}

void mouseButton(int button, int state, int x, int y) {
	win.mouseButton(button,state,x,y);
}


void mouseMoved(int x, int y) {
	win.mouseMove(x,y);
}

void passiveMouseMoved(int x, int y){
	win.passiveMouseMove(x,y);
}

void keyDown (unsigned char key, int x, int y) {
	win.keyDown(key,x,y);
}

void keyUp(unsigned char key, int x, int y){
	win.keyUp(key,x,y);
}
