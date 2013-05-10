#include "MyWindow.h"

const char* title = "Picking";

struct Cube : public Pickable {
	Uniform3f color;
	Cube(int id, vec3 pos, vec3 color, SHADER_PARAMS):Pickable(translate(mat4(1),pos), id, SHADER_CALL){
		vao = Shapes::cube();
		Shader::enable();
		this->color("color",this,color);
	}
};


struct Picking : public Viewport, public Scene, public FPInput{
	vector<Object*> objects;
	CubeBackground background;
	CubeMap* backgrounds[3];
	
	Framebuffer* pickFB;

	float nearPlane, farPlane, fovY;

	int dragIndex;

	Picking(float x, float y, float w, float h):
		Viewport(x,y,w,h)
		,FPInput()
		,nearPlane(.1f), farPlane(1001.f), fovY(60.f)
		,dragIndex(-1)
	{}

	void operator()(){
		Scene::operator()(this);
		Viewer::operator()(vec3(0,0,1));

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

		background(backgrounds[0]);

		pickFB = new Framebuffer();
		pickFB->bind();
		pickFB->attach(new Texture(NULL,2048,2048,GL_RGB32UI,GL_RGB_INTEGER,GL_UNSIGNED_INT));
		pickFB->finalize();
		pickFB->unbind();

		for (int i=0; i < 10; i++)
			objects.push_back(new Cube(i,(vec3(rand0_1,rand0_1,rand0_1)-vec3(.5))*10.f,vec3(rand0_1,rand0_1,rand0_1),"object.vert","object.frag"));

	}


	void setBackground(int i){
		background.set(backgrounds[i]);
	}

	void Scene::draw(){}

	void Viewport::draw(){
		printGLErrors("draw");
		frame();
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		background.draw();

		for (auto i=objects.begin(); i!=objects.end(); i++)
			(*i)->draw();
		printGLErrors("/draw");
	}

	void frame(){
		FPInput::frame();
		Scene::frame();

		if (FPInput::keys['c'])
			Scene::globals.lights[0].color = rainbowColors(.01);

		Clock::printFps(1000);
	}

	//called by the ViewportManager
	virtual void resize(){
		Scene::resize(perspective(fovY, (GLfloat)w/h, nearPlane, farPlane), w, h);
	}

	void select(int i){
		unselect();
		auto o = objects[i];
		if (dynamic_cast<Pickable*>(o)){
			((Pickable*)(o))->setSelected(1);
			dragIndex = i;
		}
	}

	void unselect(){
		if (dragIndex != -1){
			((Pickable*)(objects[dragIndex]))->setSelected(0);
			dragIndex = -1;
		}
	}

	virtual void mouseButton(int button, int state, int x, int y){
		FPInput::mouseButton(button, state, x, y);
		if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN){
			pickFB->bind();
			Viewport::enable();
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			for (auto i=objects.begin(); i!=objects.end(); i++)
				if (dynamic_cast<Pickable*>(*i))
					((Pickable*)(*i))->pickDraw();
			Viewport::disable();
			pickFB->unbind();

			uvec3 index;
			pickFB->read(&index,x,y,1,1,GL_RGB_INTEGER,GL_UNSIGNED_INT);
			cout << "clicked: " << index.r << " " << index.g << " " << index.b << endl;
			if (index.r < objects.size())
				select(index.r);
		}

		if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
			unselect();
	}

	virtual void mouseMove(int x, int y) {
		FPInput::mouseMove(x, y);

		//drag with right mouse
		if (mouseDown[1] && dragIndex != -1){
			vec2 change = mouseDelta;

			if (keys[' ']){
				objects[dragIndex]->changeWorldTransform(translate(mat4(1), Scene::viewer->forward * change.y * .01f));
			} else {
				float z = -(Scene::viewer->matrix() * objects[dragIndex]->getWorldTransform() * vec4(0,0,0,1)).z;
				float fovX = DEG(2*atan(tan(RAD(fovY/2))*(float)w/h));
				vec2 farDist = vec2(2 * z * tan(RAD(fovX/2)), 2 * z * tan(RAD(fovY/2)));

				change /= vec2(w,h);
				change *= farDist;

				vec3 right = normalize(cross(Scene::viewer->forward,vec3(0,1,0)));
				vec3 up = normalize(cross(right, Scene::viewer->forward));

				objects[dragIndex]->changeWorldTransform(translate(mat4(1), right * change.x) * translate(mat4(1), up * change.y));
			}
		}
	}

	virtual void passiveMouseMove(int x, int y){
		//inputHandler.passive
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
Picking picking(0,0,1,1);

void init(void)
{
	Clock::init();
	win();
	picking();
	win.add(&picking);
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
