#include "MyWindow.h"

const char* title = "Micah Lamb's volumez";

struct RayTracer : public Viewport, public Scene, public FPInput {
	Shader shader;
	mat4 transform;
	UniformMat4 worldTransform;
	UniformMat3 normalTransform;
	UniformMat4 viewTransform;

	UniformSampler reflections;

	CubeBackground background;
	CubeMap* backgrounds[3];

	float nearPlane, farPlane, fov;

	RayTracer(float x, float y, float w, float h):
		Viewport(x,y,w,h)
		,FPInput(4)
		,nearPlane(.1f), farPlane(1001.f), fov(60.f)
		,transform(1)
	{
		Scene::operator()(this);
		shader("rayTracer.vert", "rayTracer.frag");
		shader.enable();
		worldTransform("worldTransform",&shader);
		normalTransform("normalTransform",&shader);
		viewTransform("viewTransform",&shader);
		Scene::globals.lights[0].color = vec3(1,1,1);
		Scene::globals.lights[0].pos = vec3(0,0,25);

		glEnable(GL_CULL_FACE);

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

		reflections("reflections",&shader,backgrounds[0]);
		background(backgrounds[0]);

	}

	void setBackground(int i){
		shader.enable();
		reflections = backgrounds[i];
		background.set(backgrounds[i]);
	}

	void setWorldTransform(mat4 m){
		shader.enable();
		worldTransform = m;
		normalTransform = mat3(transpose(inverse(m)));
	}

	void Scene::draw(){}

	void Viewport::draw(){
		frame();
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		background.draw();

		shader.enable();
		viewTransform = matrix();
		Shapes::square()->draw();
	}
	
	void frame(){
		FPInput::frame();
		Scene::frame();

		if (keys['.'])
			setWorldTransform(scale(mat4(1),vec3(1) + vec3(Clock::delta)) * *worldTransform);
		if (keys[','])
			setWorldTransform(scale(mat4(1),vec3(1) - vec3(Clock::delta)) * *worldTransform);
		if (keys['c'])
			Scene::globals.lights[0].color = rainbowColors(.01);

		Clock::printFps(250);
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

		//rotate worldTransform with right mouse
		float w = 10;
		if (mouseDown[1]){
			mat4 m = rotate(mat4(1), mouseDelta.x*w*Clock::delta, vec3(0,1,0))
				* rotate(mat4(1), -mouseDelta.y*w*Clock::delta, right());
			transform = m * transform;
		}
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

void init_glui(){}

void display()
{
	glClearColor(.9f,.8f,.7f,1);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	win.draw();
	printGLErrors("/display");
}

//make life easy for now
void idle(){
	glutPostRedisplay();
	//display();
}

void reshape(int w, int h)
{
	int tx, ty, tw, th;
	GLUI_Master.get_viewport_area( &tx, &ty, &tw, &th );
	glViewport(tx,ty,tw,th);//only needed for clearing outside of win
	glScissor(tx,ty,tw,th);
	win.resize(tw,th);
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
