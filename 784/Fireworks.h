#include "MyWindow.h"

const char* title = "Fireworks";

#define MAX_PARTICLES 10000000
#define PARTICLE_LIFETIME 10.0f

#define PARTICLE_TYPE_LAUNCHER 0.0f
#define PARTICLE_TYPE_SHELL 1.0f
#define PARTICLE_TYPE_SECONDARY_SHELL 2.0f

struct Particle
{
    float Type;    
    vec3 Pos;
    vec3 Vel;    
    float LifetimeMillis;
	float Color;
};

struct Fireworks : public Viewport, public Scene, public FPSInput, public Object {
	//Shader shader;
	//UniformMat4 worldTransform;
	//UniformMat4 inverseTransform;
	//UniformMat3 normalTransform;

	Uniform1f gDeltaTimeMillis;
	Uniform1f gTime;
	UniformSampler gRandomTexture;
	UniformSampler gColorMap;

	FeedbackLoop looper;

	CubeBackground background;
	CubeMap* backgrounds[3];

	float nearPlane, farPlane, fov;

	Fireworks(float x, float y, float w, float h):
		Viewport(x,y,w,h)
		,FPSInput(&(Scene::viewer))
		,nearPlane(.1f), farPlane(1001.f), fov(60.f)
	{}

	void operator()(){
		Scene::operator()();
		Scene::viewer(vec3(0,0,1));
		//shader("volume.vert", "volume.frag");
		//shader("fireworks.vert", "fireworks.frag", "fireworks.geom");
		Object::operator()(mat4(1),"fireworks.vert", "fireworks.frag", "fireworks.geom");
		//worldTransform("worldTransform",&shader);
		//inverseTransform("inverseTransform",&shader);
		//normalTransform("normalTransform",&shader);
		gColorMap("gColorMap",this,new ILTexture("fireworks_red.jpg"));

		Scene::globals.lights[0].color = vec3(1,1,1);
		Scene::globals.lights[0].pos = vec3(0,0,25);
		glPointSize((GLfloat)10);
		glEnable(GL_POINT_SMOOTH);

		Particle* particles = new Particle[MAX_PARTICLES];
		particles[0].Type = PARTICLE_TYPE_LAUNCHER;
		particles[0].Pos = vec3(0);
		particles[0].Vel = vec3(0.0f, 0.0001f, 0.0f);
		particles[0].LifetimeMillis = 0.0f;
		particles[0].Color = 0;

		looper();
		looper.buffer(particles,sizeof(Particle)*MAX_PARTICLES);
		delete particles;
		printGLErrors("init 3");
		looper.inout("Type1",1,GL_FLOAT,sizeof(Particle));
		looper.inout("Position1",3,GL_FLOAT,sizeof(Particle),4);
		looper.inout("Velocity1",3,GL_FLOAT,sizeof(Particle),16);
		looper.inout("Age1",1,GL_FLOAT,sizeof(Particle),28);
		looper.inout("Color1",1,GL_FLOAT,sizeof(Particle),32);
		printGLErrors("init 4");
		looper.finalize("fireworks_update.vert","fireworks_update.geom");
		printGLErrors("init 5");
		looper.enable();
		gDeltaTimeMillis("gDeltaTimeMillis",&looper);
		gTime("gTime",&looper);
		printGLErrors("init 6");
		gRandomTexture("gRandomTexture",&looper,randomDirections(2048));
		printGLErrors("init 7");
		#define STR(x) #x
		#define CM(file) STR(cubemaps/clouds/##file)
		char* clouds[] = {CM(px.png),CM(nx.png),CM(py.png),CM(ny.png),CM(pz.png),CM(nz.png)};
		backgrounds[0] = new CubeMap(clouds, IL_ORIGIN_UPPER_LEFT);
		
		#define CM(file) STR(cubemaps/deadmeat/##file)
		char* deadmeat[] = {CM(px.jpg),CM(nx.jpg),CM(py.jpg),CM(ny.jpg),CM(pz.jpg),CM(nz.jpg)};
		backgrounds[1] = new CubeMap(deadmeat, IL_ORIGIN_UPPER_LEFT);

		#define CM(file) STR(cubemaps/misc/##file)
		char* oil[] = {CM(oil.jpg),CM(oil.jpg),CM(oil.jpg),CM(oil.jpg),CM(oil.jpg),CM(oil.jpg)};
		backgrounds[2] = new CubeMap(oil, IL_ORIGIN_LOWER_LEFT);

		background(backgrounds[0]);

		projection.push(perspective(fov, (GLfloat)w/h, nearPlane, farPlane));
	}

	Texture* randomDirections(int size){
		vec3* dirs = new vec3[size];
		for (int i=0; i < size; i++)
			dirs[i] = vec3(rand0_1,rand0_1,rand0_1);

		Texture1D* r = new Texture1D;
		r->operator()(dirs,size,GL_RGB,GL_RGB,GL_FLOAT);
		delete dirs;
		return r;
	}

	void setBackground(int i){
		background.set(backgrounds[i]);
	}

	/*
	void setWorldTransform(mat4 m){
		shader.enable();
		worldTransform = m;
		inverseTransform = inverse(m);
		normalTransform = mat3(transpose(inverse(m)));
	}
	*/
	void Scene::draw(){}

	void Viewport::draw(){
		printGLErrors("draw");
		frame();
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		background.draw();

		looper.loop(1);
		Shader::enable();
		glDepthMask(GL_FALSE);
		looper.draw();
		glDepthMask(GL_TRUE);
		
	}

	void frame(){
		FPSInput::frame();
		Scene::frame();

		looper.enable();
		gDeltaTimeMillis = Clock::deltaMilli;
		gTime = Clock::timeMilli;
		Clock::printFps(1000);
	}

	//called by the ViewportManager
	virtual void resize(int w, int h){
		Viewport::resize(w, h);
		projection.top() = perspective(fov, (GLfloat)w/h, nearPlane, farPlane);
	}
	
	virtual void mouseButton(int button, int state, int x, int y){
		FPSInput::mouseButton(button, state, x, y);
	}

	virtual void mouseMove(int x, int y) {
		FPSInput::mouseMove(x, y);

		//rotate worldTransform with right mouse
		float w = 5;
		if (mouseDown[1]){
			vec2 curPt = vec2(x,y)
				,change = curPt - lastPt[1];

			vec3 right = normalize(cross(Scene::viewer.pos,vec3(0,1,0)));

			mat4 m = rotate(mat4(1), change.x*w*elapsedTime, vec3(0,1,0))
				* rotate(mat4(1), change.y*w*elapsedTime, right);
			setWorldTransform(m * getWorldTransform());
			lastPt[1] = curPt;
		}

	}

	virtual void passiveMouseMove(int x, int y){
		//inputHandler.passive
	}

	virtual void keyDown (unsigned char key, int x, int y) {
		FPSInput::keyDown(key, x, y);
		
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
			case 'p':
				setWorldTransform(mat4(1));
		}
	}

	virtual void keyUp(unsigned char key, int x, int y){
		FPSInput::keyUp(key, x, y);
	}

};


MyWindow win;
Fireworks fireworks(0,0,1,1);

void init(void)
{
	Clock::init();
	win();
	fireworks();
	win.add(&fireworks);
}

void init_glui(){}

void display()
{
	Clock::frame();
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
