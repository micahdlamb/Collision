#include "MyWindow.h"
#include "MyFireworks.h"
#include "BallManager.h"
const char* title = "Glass Balls";

const int NUMOBJECTS = 10;
#define NUM_BUFFERS GLUT_SINGLE

#define GRAVITY vec3(0,-100.f,0)
#define VIEWER_RADIUS .5

struct Ballz : public Viewport, public Scene, public FPInput {
	struct Spheres : public UniformBlock {
		struct Sphere {
			vec3 color;
			float pad1;
			vec3 center;
			float radius;
		};
		GLint numSpheres;
		vec3 pad;
		Sphere spheres[10];

		void operator()(){
			UniformBlock::operator()(sizeof(Spheres)-sizeof(UniformBlock),10);
		}

		//push everything, write individual setters eventually
		void sync(){
			set(&numSpheres);
		}

		Sphere& operator[](int i){
			return spheres[i];
		}

	} spheres;

	UniformMat4 viewTransform;
	float nearPlane, farPlane, fovY;

	Shader shader;
	vec3 bounds;

	UniformSampler reflections;
	UniformSampler perlin;
	Uniform1f noise;
	Uniform1f rIndex;
	Uniform1f alpha;

	CubeBackground background;
	CubeMap* backgrounds[3];
	Light* light;

	BallManager balls;

	Framebuffer pickFb;
	int dragIndex;

	Ballz(float x, float y, float w, float h):
		Viewport(x,y,w,h)
		,FPInput(10)
		,nearPlane(.1f), farPlane(1000.f), fovY(60.f)
		,dragIndex(-1)
	{}

	void operator()(){
		printGLErrors("init");

		Scene::operator()(this);
		spheres();
		shader("rayTracer.vert", "glass-balls.frag");
		shader.enable();
		shader.addUniformBlock("Spheres", 10);
		viewTransform("viewTransform",&shader);
		noise("noise",&shader,.001);
		rIndex("rIndex",&shader,1.2);
		alpha("alpha",&shader,1);
		perlin("perlin",&shader,new ILTexture("MyTerrain/noise.png"));

		light = new Light(vec3(1,1,1), 2048, perspective(35.f, 1.f, 160.f, 250.f), glm::lookAt(vec3(125,160,-80),vec3(0,0,0),vec3(0,1,0)));
		Scene::globals.lights[0].color = vec3(1,1,1);
		Scene::globals.lights[0].pos = vec3(25,25,25);
		bounds = vec3(25,50,25);
		balls(-vec2(bounds.x,bounds.z), vec2(bounds.x,bounds.z), uvec2(2,2));

		pickFb(new Texture(NULL,2048,2048,GL_RGB32UI,GL_RGB_INTEGER,GL_UNSIGNED_INT));

		printGLErrors("create objects");
		for (int i=0; i < NUMOBJECTS; i++){
			float size = 3.f + rand0_1 * 6.f;
			vec3 pos = vec3((rand0_1-.5)*bounds.x, rand0_1 * bounds.y, (rand0_1-.5)*bounds.z);
			VAO* vao = Shapes::sphere(3);
			balls.add(new Ball(vao,&light->depth,i,pos,size,vec3(rand0_1,rand0_1,rand0_1),"object.vert","object.frag"));
		}
		printGLErrors("/create objects");


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

		shader.enable();
		reflections("reflections",&shader,backgrounds[0]);
		background(backgrounds[0]);

		printGLErrors("/init");
	}

	void setBackground(int i){
		shader.enable();
		reflections = backgrounds[i];
		background.set(backgrounds[i]);
	}

	void Viewport::draw(){
		printGLErrors("draw");
		frame();
		FPInput::frame();
		syncObjects();
		Scene::frame();

		glClear(GL_DEPTH_BUFFER_BIT);
		((Scene*)this)->draw();

		Clock::printFps(500);
		printGLErrors("/draw");
	}

	void Scene::draw(){
		shader.enable();
		viewTransform = matrix();
		Shapes::square()->draw();
		//drawObjects();
	}

	void frame(){
		shader.enable();
		if (keys['.'])
			noise = std::min(2.f, *noise + .2f*Clock::delta);
		if (keys[','])
			noise = std::max(0.f, *noise - .2f*Clock::delta);
			
		if (keys['c'])
			Scene::globals.lights[0].color = rainbowColors(.01);
		if (keys['\''])
			alpha = std::min(1.f, *alpha + 1.f*Clock::delta);
		if (keys[';'])
			alpha = std::max(0.1f, *alpha - 1.f*Clock::delta);
		if (keys['l'])
			rIndex = std::min(2.f, *rIndex + .3f*Clock::delta);
		if (keys['k'])
			rIndex = *rIndex - .3*Clock::delta;
	}

	void moveObjects(){
		balls.animate(Clock::delta, GRAVITY);
		balls.collide();
		balls.collideWithViewer(Viewer::pos, FPInput::vel, VIEWER_RADIUS);
		balls.collideWithWalls(bounds);
	}

	//update shaders with new object positions
	void syncObjects(){
		balls.sync();

		spheres.numSpheres = balls.objects.size();
		for (U i=0; i < balls.objects.size(); i++){
			spheres[i].center = balls.objects[i]->pos;
			spheres[i].color = *balls.objects[i]->color;
			spheres[i].radius = dynamic_cast<BoundingSphere*>(balls.objects[i]->boundingVolume)->radius;
		}
		spheres.sync();
	}


	void select(int i){
		unselect();
		auto o = balls.objects[i];
		o->wakeUp();
		o->setSelected(1);
		o->vel = vec3(0);
		dragIndex = i;
	}

	void unselect(){
		if (dragIndex != -1){
			((Pickable*)(balls.objects[dragIndex]))->setSelected(0);
			dragIndex = -1;
		}
	}

	//called by the ViewportManager
	virtual void resize(){
		Scene::resize(perspective(fovY, (GLfloat)w/h, nearPlane, farPlane), w, h);
	}

	virtual void mouseButton(int button, int state, int x, int y){
		FPInput::mouseButton(button, state, x, y);

		if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN){
			pickFb.bind();
			Viewport::enable();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			balls.pickDraw();
			Viewport::disable();
			pickFb.unbind();

			uvec3 index;
			pickFb.read(&index,x,y,1,1,GL_RGB_INTEGER,GL_UNSIGNED_INT);
			//cout << "clicked: " << index.r << " " << index.g << " " << index.b << endl;
			if (index.r < balls.objects.size())
				select(index.r);
		}

		if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
			unselect();
	}

	virtual void mouseMove(int x, int y) {
		FPInput::mouseMove(x, y);

		//drag with right mouse
		if (mouseDown[1] && dragIndex != -1){
			auto o = balls.objects[dragIndex];
			vec3 move;
			if (keys['r']){
				move = forward * mouseDelta.y * .015f;
			} else {
				//move perpendicular to screen
				float z = -(matrix() * o->getWorldTransform() * vec4(0,0,0,1)).z;
				float fovX = glm::degrees(2*atan(tan(radians(fovY/2))*(float)w/h));
				vec2 farDist = vec2(2 * z * tan(radians(fovX/2)), 2 * z * tan(radians(fovY/2)));
				vec2 change = mouseDelta;
				change /= vec2(w,h);
				change *= farDist;

				move = right() * change.x + up() * change.y;
			}
			o->pos += move;
			o->vel = move / Clock::delta;
			o->commit();
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
			case 'o':
				origin();
				vel = vec3(0);
				break;
			case 'i':
				cout << "intersections: " << balls.grid.intersections << endl;
				break;
			case 'p':
				cout << pos.x << "," << pos.y << "," << pos.z << endl;
				break;
		}
	}

	virtual void keyUp(unsigned char key, int x, int y){
		FPInput::keyUp(key, x, y);
	}

};

MyWindow win;
Ballz ballz(0,0,1,1);

void init(void)
{
	win();
	ballz();
	win.add(&ballz);
}

void init_glui(){}

void display()
{
	win.draw();
	printGLErrors("/display");
}

void idle(){
	#pragma omp parallel
	{
		#pragma omp master
		glutPostRedisplay();
		#pragma omp single
		ballz.moveObjects();
	}
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
