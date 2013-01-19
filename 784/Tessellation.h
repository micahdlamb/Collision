#include "MyWindow.h"
#include "MyFireworks.h"
#include "BallManager.h"
const char* title = "Tessellation";

const int NUMOBJECTS = 500;

struct Tessellation : public Viewport, public Scene, public TerrainWalker {
	CubeBackground background;
	CubeMap* backgrounds[3];

	Light* light;

	vec2 bounds;
	vec3 terrainDim;
	MyTerrain* terrain;

	MyFireworks fireworks;
	Chase chase;

	float nearPlane, farPlane, fovY;

	bool wireframe;

	//water reflection
	Framebuffer reflectionFb;
	Texture* reflection;
	TexturedSquare tex;

	BallManager balls;

	Framebuffer pickFb;
	int dragIndex;

	Ball* boundingSphere;
	float gravity;

	Tessellation(float x, float y, float w, float h):
		Viewport(x,y,w,h)
		,nearPlane(.1f), farPlane(1000.f), fovY(60.f)
		,wireframe(false)
		,dragIndex(-1)
	{}

	void operator()(){
		printGLErrors("init");
		if (atoi((const char*)glGetString(GL_VERSION)) < 4)
			error("OpenGL 4+ required for tessellation shaders");

		Scene::operator()(this);

		light = new Light(vec3(1,1,1), 2048, perspective(35.f, 1.f, 160.f, 250.f), glm::lookAt(vec3(125,160,-80),vec3(0,0,0),vec3(0,1,0)));

		glEnable(GL_CULL_FACE);

		fireworks(mat4(25));
		chase(mat4(1));

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

		reflection = new Texture(NULL,2048,2048,GL_RGB);
		reflectionFb(reflection);

		tex(&light->depth);

		GLint MaxPatchVertices = 0;
		glGetIntegerv(GL_MAX_PATCH_VERTICES, &MaxPatchVertices);
		printf("Max supported patch vertices %d\n", MaxPatchVertices);

		glPatchParameteri(GL_PATCH_VERTICES, 4);
		terrainDim = vec3(100,15,100);
		terrain = new MyTerrain("valley1.hm",scale(mat4(1),terrainDim),/*backgrounds[2]*/reflection,25, &light->depth);
		bounds = vec2(terrainDim.x,terrainDim.z) * .5f;

		TerrainWalker::operator()(terrain,10,.1,2,15,40);
		balls(-bounds, bounds, uvec2(25,25));

		pickFb(new Texture(NULL,2048,2048,GL_RGB32UI,GL_RGB_INTEGER,GL_UNSIGNED_INT));

		printGLErrors("create objects");
		for (int i=0; i < NUMOBJECTS; i++){
			float size = .5f + rand0_1 * .75f;
			vec3 pos((rand0_1-.5)*terrainDim.x,0,(rand0_1-.5)*terrainDim.z);
			pos.y = terrain->getY(pos.x,pos.z) + size;
			VAO* vao;
			int r = rand() % 4;
			switch(r){
				case 0:
					vao = Shapes::sphere(3);
					break;
				case 1:
					vao = Shapes::file("OFF/models/mushroom.off");
					break;
				case 2:
					vao = Shapes::sphere(3);
					break;
				case 3:
					size /= 10;
					vao = Shapes::file("OFF/models/head.off");
					break;
				default: {
					/*
					size = .2f;
					string l = string("") + (char)('a' + (r - 2));
					l = string("off/letters/") + l + ".off";
					vao = Shapes::file((char*)l.c_str());
					*/
				}
			}
			balls.add(new Ball(vao,&light->depth,i,pos,size,vec3(rand0_1,rand0_1,rand0_1),"object.vert","object.frag"));
		}
		printGLErrors("/create objects");

		boundingSphere = new Ball(Shapes::sphere(3),&light->depth,-1,vec3(0),1,vec3(1,0,0),"object.vert","object.frag");

		background(backgrounds[2]);
		printGLErrors("/init");
	}

	void setBackground(int i){
		background.set(backgrounds[i]);
	}

	void drawShadows(){
		auto ep = eyePos();
		pushEye(light->projection, light->view);
		globals.syncEye(eye(),ep);//hack to make sure scene tessellated same in shadow map as main view frustrum
		light->bind();
		terrain->shadowDraw();
		balls.shadowDraw();
		light->unbind();
		popEye();
	}

	void drawReflection(){
		pushView(scale(matrix(),vec3(1,-1,1)));
		glFrontFace(GL_CW);
		reflectionFb.bind();
		terrain->setWaveAmp(0);//seems to help reflection holes
		((Scene*)this)->draw();
		terrain->setWaveAmp(.003);
		reflectionFb.unbind();
		glFrontFace(GL_CCW);
		popView();
	}

	void drawBoundingSpheres(){
		boundingSphere->setAsleep(1);
		for (auto i=balls.objects.begin(); i!=balls.objects.end(); i++){
			if ((*i)->vao == Shapes::sphere()) continue;
			auto s = dynamic_cast<BoundingSphere*>((*i)->boundingVolume);
			boundingSphere->setWorldTransform(translate(mat4(1),s->center) * scale(mat4(),vec3(s->radius)));
			boundingSphere->draw();
		}
	}

	void Viewport::draw(){
		printGLErrors("draw");

		frame();
		Scene::frame();
		TerrainWalker::frame();
		balls.sync();
		drawShadows();
		drawReflection();
		((Scene*)this)->draw();

		/*
		Viewport::push(0,0,300,300);
		glDisable(GL_DEPTH_TEST);
		tex.draw();
		glEnable(GL_DEPTH_TEST);
		Viewport::pop();
		*/

		Clock::printFps(500);
		printGLErrors("/draw");
	}

	void Scene::draw(){
		glClear(GL_DEPTH_BUFFER_BIT);
		background.draw();
		glPushAttrib(GL_POLYGON_BIT);
		if (wireframe) glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		terrain->draw();
		balls.draw();
		//drawBoundingSpheres();
		chase.setChasePt((balls.objects[0]->getWorldTransform() * vec4(0,0,0,1)).swizzle(X,Y,Z));
		chase.draw();
		glPopAttrib();

		fireworks.draw();
	}

	void moveObjects(){
		balls.animate(Clock::delta, TerrainWalker::gravity);
		balls.collide();
		balls.collideWithTerrain(terrain,bounds-vec2(1));
		balls.collideWithViewer(Viewer::pos - vec3(0, .5 * TerrainWalker::viewerHeight, 0), TerrainWalker::vel, TerrainWalker::viewerHeight*.5);
	}

	void frame(){
		
		if (keys['c'])
			light->color = rainbowColors(.01);

		if (keys['.'])
			terrain->setLod(*terrain->lod + .1*Clock::delta);
		if (keys[','])
			terrain->setLod(*terrain->lod - .1*Clock::delta);
		
		if (keys[';'] || keys['\'']){
			vec3 lightPos = light->pos();
			if (keys[';'])
				lightPos = vec3(rotate(mat4(1),-20*Clock::delta,vec3(1,0,1)) * vec4(lightPos,1));
			else if (keys['\''])
				lightPos = vec3(rotate(mat4(1),20*Clock::delta,vec3(1,0,1)) * vec4(lightPos,1));
			light->view = lookAt(lightPos,vec3(0,0,0),vec3(0,1,0));
		}

		Scene::globals.lights[0].color = light->color;
		Scene::globals.lights[0].pos = light->pos();
		Scene::globals.lights[0].eye = light->eye();

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
		TerrainWalker::mouseButton(button, state, x, y);

		if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN){
			pickFb.bind();
			Viewport::enable();
			glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
		TerrainWalker::mouseMove(x, y);

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
		TerrainWalker::keyDown(key, x, y);
		
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
			case ' ':
				wireframe = !wireframe;
				break;
			case 'f':
				jump();
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
		TerrainWalker::keyUp(key, x, y);
	}

};

MyWindow win;
Tessellation tessellation(0,0,1,1);

void init(void)
{
	tessellation();
	win();
	win.add(&tessellation);
	Clock::maxDelta = 1.f/55;
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
		tessellation.moveObjects();
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
