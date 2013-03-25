#include "MyWindow.h"
#include "MyFireworks.h"
#include "bullet/btBulletDynamicsCommon.h"
#include "bullet/btBulletCollisionCommon.h"
#include "bullet/BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h"

const char* title = "Tessellation";

const int NUMOBJECTS = 100;

btVector3 btVec3(vec3 v){
	return btVector3(v.x,v.y,v.z);
}

struct Thing : public Pickable, public btMotionState{
	Uniform3f color;
	UniformSampler shadowMap;
	vec3 scale;
	btRigidBody* rigidBody;
	Thing(VAO* vao, Texture* shadowMap, int id, vec3 pos, float size, vec3 color, SHADER_PARAMS):
		scale(vec3(size))
		,Pickable(glm::translate(mat4(1),pos)*glm::scale(mat4(1),scale), id, SHADER_CALL)
	{
		setVAO(vao);
		this->shadowMap("shadowMap",this,shadowMap);
		this->color("color",this,color);
		enableShadowCast();
	}

	void getWorldTransform(btTransform& worldTrans) const{
		//fuck const
		worldTrans.setFromOpenGLMatrix((btScalar*)&Object::shared.v.worldTransform);
	}

	void setWorldTransform(const btTransform& worldTrans){
		mat4 m;
		worldTrans.getOpenGLMatrix((btScalar*)&m);
		Object::setWorldTransform(m * glm::scale(mat4(1),scale));
	}

	void updateBV(){
		vao->boundingVolume->transform(Object::getWorldTransform(), boundingVolume);
	}
};

struct Tessellation : public Viewport, public Scene, public TerrainWalker, public btMotionState{
	CubeBackground background;
	CubeMap* backgrounds[3];

	Light* light;

	vec3 terrainDim;
	MyTerrain* terrain;

	MyFireworks fireworks;

	float nearPlane, farPlane, fovY;

	bool wireframe;

	//water reflection
	Framebuffer reflectionFb;
	Texture* reflection;
	TexturedSquare tex;

	//pickable cubes
	vector<Thing*> objects;
	Framebuffer pickFb;
	int dragIndex;
	vec3 dragVel;

	//Bullet
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* overlappingPairCache;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;

	btRigidBody* eyeBall;

	void getWorldTransform(btTransform& worldTrans) const{
		//cout <<"getWorldTransform"<<endl;
		worldTrans.setOrigin(btVec3(Viewer::pos - vec3(0,viewerHeight/2,0)));
		//if (eyeBall) eyeBall->setLinearVelocity(btVec3(TerrainWalker::vel));
	}

	void setWorldTransform(const btTransform& worldTrans){
		//cout <<"setWorldTransform"<<endl;
		//worldTrans.setOrigin(btVec3(Viewer::pos));
	}

	Tessellation(float x, float y, float w, float h):
		Viewport(x,y,w,h)
		,nearPlane(.1f), farPlane(1000.f), fovY(60.f)
		,wireframe(false)
		,dragIndex(-1)
	{
		printGLErrors("init");
		if (atoi((const char*)glGetString(GL_VERSION)) < 4)
			error("OpenGL 4+ required for tessellation shaders");

		Scene::operator()(this);
		light = new Light(vec3(1,1,1), 1024, perspective(45.f, 1.f, 50.f, 250.f), glm::lookAt(vec3(0,125,-100),vec3(0,0,0),vec3(0,1,0)));
		glEnable(GL_CULL_FACE);
		fireworks(mat4(25));

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

		GLint maxPatchVertices = 0;
		glGetIntegerv(GL_MAX_PATCH_VERTICES, &maxPatchVertices);
		printf("Max supported patch vertices %d\n", maxPatchVertices);

		glPatchParameteri(GL_PATCH_VERTICES, 4);
		terrainDim = vec3(100,15,100);
		terrain = new MyTerrain("valley1.hm", scale(mat4(1),terrainDim), reflection, 25, &light->depth);

		TerrainWalker::operator()(terrain,10,.1,2,15,25);

		pickFb(new Texture(NULL,2048,2048,GL_RGB32UI,GL_RGB_INTEGER,GL_UNSIGNED_INT));

		///collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
		btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();

		///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
		btCollisionDispatcher* dispatcher = new	btCollisionDispatcher(collisionConfiguration);

		///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
		btBroadphaseInterface* overlappingPairCache = new btDbvtBroadphase();

		///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
		btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

		dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration);

		dynamicsWorld->setGravity(btVector3(0,TerrainWalker::gravity.y,0));
		
		//add terrain collision
		auto& heights = terrain->heights;
		auto tcollider = new btHeightfieldTerrainShape(heights.rows,heights.cols,heights.v,1,0,1,1,PHY_FLOAT,false);
		tcollider->setUseDiamondSubdivision(true);
		tcollider->setLocalScaling((btVector3(terrainDim.x/heights.cols, terrainDim.y, terrainDim.z/heights.rows)));

		auto ground = new btRigidBody(0,new btDefaultMotionState(),tcollider);
		//ground->setCenterOfMassTransform(t);
		ground->getWorldTransform().setRotation(btQuaternion(PI/2,0,0));
		ground->getWorldTransform().setOrigin(btVector3(0,terrainDim.y/2,0));
		dynamicsWorld->addRigidBody(ground);

		vector<GLuint> indices;
		vector<vec3> vertices;
		loadOFF("OFF/models/mushroom.off",vertices,indices);
		btCollisionShape* mushroom = new btConvexHullShape((const btScalar*)&vertices[0], vertices.size(), sizeof(vec3));
		vertices.clear();
		loadOFF("OFF/models/head.off",vertices,indices);
		btCollisionShape* head = new btConvexHullShape((const btScalar*)&vertices[0], vertices.size(), sizeof(vec3));
		btCollisionShape* collider;

		printGLErrors("create objects");
		for (int i=0; i < NUMOBJECTS; i++){
			float size = .5f + rand0_1 * .75f;
			vec3 pos((rand0_1-.5)*terrainDim.x,0,(rand0_1-.5)*terrainDim.z);
			pos.y = terrain->getY(pos.x,pos.z) + size;
			VAO* vao;
			int r = rand() % 2;
			string file = "";
			switch(r){
				case 0:
					collider = new btSphereShape(1);
					vao = Shapes::sphere(3);
					break;
				case 1:
					collider = mushroom;
					vao = Shapes::file("OFF/models/mushroom.off");
					break;
				case 2:
					collider = new btSphereShape(1);
					vao = Shapes::sphere(3);
					break;
				case 3:
					size /= 10;
					collider = head;
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

			auto thing = new Thing(vao,&light->depth,i,pos,size,vec3(rand0_1,rand0_1,rand0_1),"object.vert","object.frag");
			objects.push_back(thing);

			collider->setLocalScaling(btVector3(size,size,size));
			btVector3 inertia(0,0,0);
			collider->calculateLocalInertia(size, inertia);
			auto body = new btRigidBody(size, thing, collider, inertia);
			dynamicsWorld->addRigidBody(body);
			thing->rigidBody = body;
		}

		auto eyeShape = new btCapsuleShape(1, TerrainWalker::viewerHeight);
		eyeBall = new btRigidBody(1000,this,eyeShape);
		eyeBall->setSleepingThresholds(0,0);
		dynamicsWorld->addRigidBody(eyeBall);

		printGLErrors("/create objects");
		background(backgrounds[2]);
		printGLErrors("/init");
	}

	void setBackground(int i){
		background.set(backgrounds[i]);
	}

	void drawShadows(){
		auto ep = eyePos();
		light->bind();
		globals.syncEye(eye(),ep);//hack to make sure scene tessellated same in shadow map as main view frustrum
		terrain->shadowDraw();
		for (auto i=objects.begin(); i!=objects.end(); i++)
			(*i)->shadowDraw();
		light->unbind();
	}

	void drawReflection(){
		pushView(matrix()*scale(mat4(1),vec3(1,-1,1)));
		glFrontFace(GL_CW);
		reflectionFb.bind();
		terrain->setWaveAmp(0);//seems to help reflection holes
		((Scene*)this)->draw();
		terrain->setWaveAmp(.003);
		reflectionFb.unbind();
		glFrontFace(GL_CCW);
		popView();
	}

	void drawObjects(){
		for (auto i=objects.begin(); i!=objects.end(); i++)
			(*i)->draw();
	}

	void Viewport::draw(){
		printGLErrors("draw");

		frame();
		Scene::frame();
		TerrainWalker::frame();

		drawShadows();
		drawReflection();
		((Scene*)this)->draw();

		Clock::printFps(500);
		printGLErrors("/draw");
	}

	void Scene::draw(){
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		background.draw();
		glPushAttrib(GL_POLYGON_BIT);
		if (wireframe) glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		terrain->draw();
		drawObjects();
		//drawBoundingSpheres();
		glPopAttrib();

		fireworks.draw();
	}

	void frame(){
		eyeBall->setLinearVelocity(btVec3(TerrainWalker::vel));
		//btTransform t;
		//t.setOrigin(btVec3(Viewer::pos));
		auto rpos = eyeBall->getCenterOfMassPosition();
		eyeBall->translate(btVec3(Viewer::pos) - rpos);

		dynamicsWorld->stepSimulation(Clock::delta,10);
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
		auto o = objects[i];
		o->setSelected(1);
		dragIndex = i;
		o->rigidBody->setGravity(btVector3(0,0,0));
	}

	void unselect(){
		if (dragIndex != -1){
			auto o = objects[dragIndex];
			o->setSelected(0);
			o->rigidBody->setLinearVelocity(btVector3(dragVel.x, dragVel.y, dragVel.z));
			auto& g = TerrainWalker::gravity;
			o->rigidBody->setGravity(btVector3(g.x,g.y,g.z));
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
			for (auto i=objects.begin(); i!=objects.end(); i++)
				if (dynamic_cast<Pickable*>(*i))
					((Pickable*)(*i))->pickDraw();
			Viewport::disable();
			pickFb.unbind();

			uvec3 index;
			pickFb.read(&index,x,y,1,1,GL_RGB_INTEGER,GL_UNSIGNED_INT);
			if (index.r < objects.size())
				select(index.r);
		}

		if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP)
			unselect();
	}

	virtual void mouseMove(int x, int y) {
		TerrainWalker::mouseMove(x, y);

		//drag with right mouse
		if (mouseDown[1] && dragIndex != -1){
			auto o = objects[dragIndex];
			vec3 move;
			if (keys['r']){
				move = forward * mouseDelta.y * .025f;
			} else {
				//move perpendicular to screen
				float z = -(matrix() * objects[dragIndex]->Object::getWorldTransform() * vec4(0,0,0,1)).z;
				float fovX = glm::degrees(2*atan(tan(radians(fovY/2))*(float)w/h));
				vec2 farDist = vec2(2 * z * tan(radians(fovX/2)), 2 * z * tan(radians(fovY/2)));
				vec2 change = mouseDelta;
				change /= vec2(w,h);
				change *= farDist;

				move = right() * change.x + up() * change.y;
			}
			o->rigidBody->activate(true);
			o->rigidBody->translate(btVector3(move.x,move.y,move.z));
			dragVel = move / Clock::delta;
			o->rigidBody->setLinearVelocity(btVector3(0,0,0));
			//o->pos += move;
			//o->vel = move / Clock::delta;
			//o->commit();
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
Tessellation* tessellation;

void init(void)
{
	win();
	tessellation = new Tessellation(0,0,1,1);
	win.add(tessellation);
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
