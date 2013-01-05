#include "MyWindow.h"

const char* title = "Micah Lamb's Cubez";


struct Cubez : public Viewport, public Scene, public FPSInput {
	Shader shader;
	mat4 transform;
	UniformMat4 worldTransform;
	UniformMat3 normalTransform;
	Uniform1i pattern;

	Uniform1i reflectionsOn;
	Uniform1i blur;
	UniformSampler reflections;
	int pause;

	static const int NUMCUBES = 43;
	CubeMap* cubeMaps[NUMCUBES];

	CubeBackground background;
	CubeMap* backgrounds[3];

	float nearPlane, farPlane, fov;

	Cubez(float x, float y, float w, float h):
		Viewport(x,y,w,h)
		,FPSInput(&(Scene::viewer))
		,nearPlane(.1f), farPlane(1001.f), fov(60.f)
		,transform(1)
		,pause(0)
	{
	}

	void operator()(){
		Scene::operator()();
		Scene::viewer(vec3(0,0,1));
		shader("surface.vert", "surface.frag");
		shader.enable();
		worldTransform("worldTransform",&shader);
		normalTransform("normalTransform",&shader);
		pattern("pattern",&shader);
		reflectionsOn("reflectionsOn",&shader,0);
		blur("blur",&shader,0);
		Scene::globals.lights[0].color = vec3(0,1,0);
		Scene::globals.lights[0].pos = vec3(0,0,25);

		glEnable(GL_CULL_FACE);

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

		reflections("reflections",&shader,NULL);
		background(backgrounds[0]);
		
		for (int i=0; i < NUMCUBES; i++)
			cubeMaps[i] = new CubeMap(256);

		projection.push(perspective(fov, (GLfloat)w/h, nearPlane, farPlane));
	}
	void toggleReflections(){
		shader.enable();
		reflectionsOn = *reflectionsOn ? 0 : 1;
	}
	void setPattern(int pattern){
		shader.enable();
		this->pattern = pattern;
	}

	void setBlur(int i){
		shader.enable();
		blur = clamp(i,0,8);
	}

	void setBackground(int i){
		shader.enable();
		background.set(backgrounds[i]);
	}

	void setWorldTransform(mat4 m){
		shader.enable();
		m =  transform * m;
		worldTransform = m;
		normalTransform = mat3(transpose(inverse(m)));
	}

	void Viewport::draw(){
		draw(true);
	}

	void Scene::draw(){
		draw(false);
	}

	void draw(bool genMaps){
		
		if (genMaps){
			FPSInput::frame();
			
		}
		Scene::frame();
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		background.draw();

		shader.enable();
		int i=0;
		cubez(mat4(1), genMaps, i);
	}
	
	void cubez(mat4 transform, bool genMaps, int &i, int depth=0){
		printGLErrors("begin cubez");
		int c[] = {1,7,43,259,1555};
		if (c[depth] > NUMCUBES)//-1 to eliminate float rounding
			return;

		if (i >= NUMCUBES){
			cout << "i to high: " << i << " depth: " << depth << endl;
			return;
		}

		if (genMaps && !pause)
			cubeMaps[i]->update(this, vec3(transform * vec4(0,0,0,1)), (mat3(transform) * vec3(.51,0,0)).x);

		reflections = cubeMaps[i];
		setWorldTransform(transform);
		Shapes::cube()->draw();

		transform = transform * scale(mat4(1), vec3(.5));
		float time = (pause ? pause : glutGet(GLUT_ELAPSED_TIME))/1000.f;
		float t = sin(time*.35f)*4;//(3.f/(depth*depth+1));
		i++;
		cubez(transform * translate(mat4(1),vec3(-t,0,0)), genMaps, i, depth+1);
		cubez(transform * translate(mat4(1),vec3(t,0,0)), genMaps, i, depth+1);
		cubez(transform * translate(mat4(1),vec3(0,-t,0)), genMaps, i, depth+1);
		cubez(transform * translate(mat4(1),vec3(0,t,0)), genMaps, i, depth+1);
		cubez(transform * translate(mat4(1),vec3(0,0,-t)), genMaps, i, depth+1);
		cubez(transform * translate(mat4(1),vec3(0,0,t)), genMaps, i, depth+1);
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
		if (mouseDown[1]){
			vec2 curPt = vec2(x,y)
				,change = curPt - lastPt[1];

			vec3 right = normalize(cross(Scene::viewer.pos,vec3(0,1,0)));

			mat4 m = rotate(mat4(1), change.x*2.f*elapsedTime, vec3(0,1,0))
				* rotate(mat4(1), change.y*2.f*elapsedTime, right);
			transform = m * transform;
			lastPt[1] = curPt;
		}

	}

	virtual void passiveMouseMove(int x, int y){
		//inputHandler.passive
	}

	virtual void keyDown (unsigned char key, int x, int y) {
		FPSInput::keyDown(key, x, y);
		
		switch (key){
			case 'r':
				toggleReflections();
				break;
			case '1':
				setBackground(0);
				break;
			case '2':
				setBackground(1);
				break;
			case '3':
				setBackground(2);
				break;
			case '.':
				setPattern((*pattern+1) % 4);
				break;
			case ',':
				setPattern(glm::mod(*pattern-1.f,4.f));
				break;
			case '<':
				setBlur(*blur-1);
				break;
			case '>':
				setBlur(*blur+1);
				break;
			case 'p':
				pause = pause ? 0 : glutGet(GLUT_ELAPSED_TIME);
				break;
		}
	}

	virtual void keyUp(unsigned char key, int x, int y){
		FPSInput::keyUp(key, x, y);
	}

};


MyWindow win;
Cubez cubez(0,0,1,1);


void init(void)
{
	win();
	cubez();
	win.add(&cubez);
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
