#include "MyWindow.h"
#include "MyFireworks.h"

const char* title = "Tessellation";

const int NUMOBJECTS = 500;

struct Ball {
	Ball(vec3 pos, vec3 vel, float mass):pos(pos),vel(vel),mass(mass){}
	vec3 pos, vel;
	float mass;
};

struct Thing : public Pickable, public Ball {
	Uniform3f color;
	Uniform1i asleep;
	UniformSampler shadowMap;
	
	float still;
	int sleepTicks;
	int stillTicks;

	Thing(VAO* vao, Texture* shadowMap, int id, vec3 pos, float size, vec3 color, SHADER_PARAMS):
		Pickable(translate(mat4(1),pos)*scale(mat4(1),vec3(size)), id, SHADER_CALL)
		,Ball(pos,vec3(0),0)
		,still(.5)
		,stillTicks(0)
		,sleepTicks(100)
	{
		setVAO(vao);
		asleep("asleep",this,0);
		this->shadowMap("shadowMap",this,shadowMap);
		mass = dynamic_cast<BoundingSphere*>(boundingVolume)->radius;
		this->color("color",this,color);
		enableShadowCast();
	}
	void move(vec3 acc=vec3(0)){
		if (trySleep()) return;
		vel += acc * Clock::delta;
		pos += vel * Clock::delta;
		updateBV();
	}

	mat4 getWorldTransform(){
		mat4 m = Object::getWorldTransform();
		m[3] = vec4(pos,1);//only change translation part
		return m;
	}

	void updateBV(){
		vao->boundingVolume->transform(getWorldTransform(), boundingVolume);
	}

	void commit(){
		if (NaN(pos.x) || NaN(vel.x)){
			cout << "wierd ball fuck up" << endl;
			pos = vec3(0,100,0);
			vel = vec3(0);
		}

		if (!sleeping())
			setWorldTransform(getWorldTransform());
		setAsleep(sleeping()?1:0);
	}

	void setAsleep(int v){
		Shader::enable();
		asleep = v;
	}

	/*
	handle the sleep optimization
	once object is moving slower than still for more than sleepTicks frames set it to be asleep
	Once asleep:
		the object is no longer affected by gravity or moves, no need to update bounding volume
		no need to check for intersection between 2 sleeping objects
		no more need to check for intersection with terrain and compute reflections
	*/
	bool isStill(){
		return dot(vel,vel) < still;
	}

	bool trySleep(){
		if (isStill())
			stillTicks++;
		else
			wakeUp();

		Object::boundingVolume->sleeping = sleeping();
		if (sleeping())
			vel = vec3(0);
		return sleeping();
	}

	bool sleeping(){
		return stillTicks > sleepTicks;
	}
	
	void wakeUp(){
		stillTicks = 0;
		Object::boundingVolume->sleeping = false;
	}
};

struct Tessellation : public Viewport, public Scene, public TerrainWalker, public Grid2D::IntersectionHandler {
	CubeBackground background;
	CubeMap* backgrounds[3];

	Light* light;

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

	//pickable cubes
	vector<Thing*> objects;
	vector<IBoundingVolume*> bvs;
	Framebuffer pickFb;
	int dragIndex;

	Thing* boundingSphere;

	Grid2D grid;

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
		
		vec2 bounds = vec2(terrainDim.x,terrainDim.z) * .5f;
		grid(-bounds,bounds,uvec2(25,25));

		TerrainWalker::operator()(terrain,10,.1,2,15,25);

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
			auto thing = new Thing(vao,&light->depth,i,pos,size,vec3(rand0_1,rand0_1,rand0_1),"object.vert","object.frag");
			objects.push_back(thing);
			bvs.push_back(thing->boundingVolume);
		}
		printGLErrors("/create objects");

		boundingSphere = new Thing(Shapes::sphere(3),&light->depth,-1,vec3(0),1,vec3(1,0,0),"object.vert","object.frag");

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
		for (auto i=objects.begin(); i!=objects.end(); i++)
			(*i)->shadowDraw();
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

	void drawObjects(){
		for (auto i=objects.begin(); i!=objects.end(); i++)
			(*i)->draw();
	}

	void drawBoundingSpheres(){
		boundingSphere->setAsleep(1);
		for (auto i=objects.begin(); i!=objects.end(); i++){
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
		syncObjects();
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
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		background.draw();
		glPushAttrib(GL_POLYGON_BIT);
		if (wireframe) glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		terrain->draw();
		drawObjects();
		//drawBoundingSpheres();
		chase.setChasePt((objects[0]->getWorldTransform() * vec4(0,0,0,1)).swizzle(X,Y,Z));
		chase.draw();
		glPopAttrib();

		fireworks.draw();
	}

	void moveObjects(){
		float damp = .5;
		for (auto i=objects.begin(); i != objects.end(); i++){
			if (*(*i)->selected) continue;
			(*i)->move(TerrainWalker::gravity);
		}

		collideObjects();

		//bound & ball characteristics of the eye
		BoundingSphere eyeBound(viewer->pos - vec3(0, .5 * viewerHeight, 0), viewerHeight*.5);
		Ball eyeBall(Viewer::pos,TerrainWalker::vel,10);

		#pragma omp parallel for
		for (int i=0; i < objects.size(); i++){
			auto t1 = objects[i];

			//collide with eye
			auto r = eyeBound.intersect(t1->boundingVolume);
			if (r.intersect && dot(t1->vel - eyeBall.vel, eyeBall.pos - t1->pos) > FLT_EPSILON){
				//cout<<"eye collide"<<endl;
				collide(*t1,eyeBall,eyeBall.pos - t1->boundingVolume->center);
			}
			//TerrainWalker::vel = eyeBall.vel;

			if (t1->sleeping()) continue;

			//keep in terrain
			auto& pos = t1->pos;
			auto& vel = t1->vel;
			auto s = dynamic_cast<BoundingSphere*>(t1->boundingVolume);
			auto bounds = terrainDim * .5f - 1.f;
			if (pos.x > bounds.x){
				pos.x = bounds.x;
				vel.x *= -damp;
			}
			if (pos.x < -bounds.x){
				pos.x = -bounds.x;
				vel.x *= -damp;
			}
			if (pos.z > bounds.z){
				pos.z = bounds.z;
				vel.z *= -damp;
			}
			if (pos.z < -bounds.z){
				pos.z = -bounds.z;
				vel.z *= -damp;
			}

			float y = terrain->getY(s->center.x,s->center.z) + s->radius;
			if (s->center.y < y && dot(vel,vel) > 0){
				pos.y = y + pos.y - s->center.y;
				auto normal = terrain->getNormal(s->center.x,s->center.z);
				vel = reflect(t1->vel,normal) * (1 - dot(normalize(-vel), normal) * (1-damp));
			}
		}
	}

	//update shaders with new object positions
	void syncObjects(){
		for (auto i=objects.begin(); i != objects.end(); i++)
			(*i)->commit();
	}

	//pass bounding volumes to grid intersection routine
	void collideObjects(){
		grid.intersect(bvs, this);
	}

	//handle intersection by calling handle collision
	void handleIntersection(Grid2D::Result r){
		auto& b1 = *objects[r.first];
		auto& b2 = *objects[r.second];
		handleCollision(b1,b2);
	}

	//separate objects and make sure they are moving towards each other, then collide them
	void handleCollision(Thing& t1, Thing& t2){
		auto s1 = dynamic_cast<BoundingSphere*>(t1.boundingVolume);
		auto s2 = dynamic_cast<BoundingSphere*>(t2.boundingVolume);

		auto v = s2->center - s1->center;
		float dist = length(v);
		if (dist > 0){
			v = normalize(v);

			//separate bvs
			float overlap = s1->radius + s2->radius - dist;
			if (!t1.sleeping())
				t1.pos -= v * overlap * .5f * (dot(t1.vel,t1.vel) < 5 ? .2f : 1.f);
			if (!t2.sleeping())
				t2.pos += v * overlap * .5f * (dot(t2.vel,t2.vel) < 5 ? .2f : 1.f);

			//do nothing if movement very slow or moving away from each other
			if (dot(t1.vel - t2.vel, t2.pos - t1.pos) <= FLT_EPSILON) return;//fix balls getting stuck in each other
			collide(t1, t2, v);
		}
	}

	//conservation of momentum (without angular velocity yet)
	static void collide(Ball& b1, Ball& b2, vec3 n){
		float e = 1;
		float v1i = dot(b1.vel, n);
		float v2i = dot(b2.vel, n);

		vec3 v = b1.vel - b2.vel;
		float j = (-(1+e)*dot(v,n)) / (dot(n,n)*(1/b1.mass + 1/b2.mass));
		b1.vel += (j / b1.mass)*n;
		b2.vel -= (j / b2.mass)*n;
	}

	/*
	//conservation of momentum (without angular velocity yet)
	static void collide(Ball& b1, Ball& b2){
		float e = 1;
		auto n = b2.pos - b1.pos;//normalize(b2.pos - b1.pos);
		float v1i = dot(b1.vel, n);
		float v2i = dot(b2.vel, n);

		vec3 v = b1.vel - b2.vel;
		float j = (-(1+e)*dot(v,n)) / (dot(n,n)*(1/b1.mass + 1/b2.mass));
		b1.vel += (j / b1.mass)*n;
		b2.vel -= (j / b2.mass)*n;
	}


	static void collide2(Ball& b1, Ball& b2){
		//derived by hand
		auto n = normalize(b2.pos - b1.pos);
		float v1i = dot(b1.vel, n);
		float v2i = dot(b2.vel, n);

		float momentum = b1.mass * v1i + b2.mass * v2i;
		float energy = .5 * b1.mass * v1i * v1i + .5 * b2.mass * v2i * v2i;

		float s1, s2;
		float a = .5 * b2.mass * b2.mass  / b1.mass + .5 * b2.mass;
		float b = -momentum * b2.mass / b1.mass;
		float c = -energy + .5 * momentum * momentum / b1.mass;
		solveQuadratic(s1, s2, a, b, c);

		float v2f = s2;
		float v1f = (momentum - b2.mass * v2f) / b1.mass;

		b1.vel += (v1f - v1i) * n;
		b2.vel += (v2f - v2i) * n;
	}
	*/

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
		auto o = objects[i];
		o->wakeUp();
		o->setSelected(1);
		o->vel = vec3(0);
		dragIndex = i;
	}

	void unselect(){
		if (dragIndex != -1){
			((Pickable*)(objects[dragIndex]))->setSelected(0);
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
			//cout << "clicked: " << index.r << " " << index.g << " " << index.b << endl;
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
				cout << "intersections: " << grid.intersections << endl;
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
	glClearColor(.9f,.8f,.7f,1);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
