#include "MyWindow.h"
//#include "intrin.h"

const char* title = "Micah Lamb's Volumez";

struct Volumez : public Viewport, public Scene, public FPInput {
	Shader shader;
	UniformMat4 worldTransform;
	UniformMat4 inverseTransform;
	UniformMat3 normalTransform;
	Uniform1f delta;
	Uniform1f absorbtion;
	Uniform1f threshold;

	UniformSampler volume;
	UniformSampler normals;

	CubeBackground background;
	CubeMap* backgrounds[3];

	float nearPlane, farPlane, fovY;

	Volumez(float x, float y, float w, float h):
		Viewport(x,y,w,h)
		,nearPlane(.1f), farPlane(1001.f), fovY(60.f)
	{}

	void operator()(){
		Scene::operator()(this);
		//shader("volume.vert", "volume.frag");
		shader("brain.vert", "brain.frag");
		shader.enable();
		worldTransform("worldTransform",&shader);
		inverseTransform("inverseTransform",&shader);
		normalTransform("normalTransform",&shader);
		delta("delta",&shader,.005);
		absorbtion("absorbtion",&shader,10);
		threshold("threshold",&shader,.01);
		Scene::globals.lights[0].color = vec3(1,1,1);
		Scene::globals.lights[0].pos = vec3(0,0,25);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		//volume("volume",&shader,gen3dTexture());
		Texture *v, *n;
		load3dBrain(v,n);
		volume("volume",&shader,v);
		normals("normals",&shader,n);

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
	}

	Texture* gen3dTexture(){
		int VOLUME_TEX_SIZE = 128;
		int size = VOLUME_TEX_SIZE*VOLUME_TEX_SIZE*VOLUME_TEX_SIZE* 4;
		GLubyte *data = new GLubyte[size];

		for(int x = 0; x < VOLUME_TEX_SIZE; x++)
		{for(int y = 0; y < VOLUME_TEX_SIZE; y++)
		{for(int z = 0; z < VOLUME_TEX_SIZE; z++)
		{
			data[(x*4)   + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = z*2;
			data[(x*4)+1 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = y*2;
			data[(x*4)+2 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 250;
			data[(x*4)+3 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 25;
	  	
			vec3 p = vec3(x,y,z)- vec3(VOLUME_TEX_SIZE-20,VOLUME_TEX_SIZE-30,VOLUME_TEX_SIZE-30);
			bool test = (length(p) < 42);
			//test = false;
			if(test)
				data[(x*4)+3 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 0;

			p =	vec3(x,y,z)- vec3(VOLUME_TEX_SIZE/2,VOLUME_TEX_SIZE/2,VOLUME_TEX_SIZE/2);
			test = (length(p) < 24);
			//test = false;
			if(test)
				data[(x*4)+3 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 0;

		
			if(x > 20 && x < 40 && y > 0 && y < VOLUME_TEX_SIZE && z > 10 &&  z < 50)
			{
			
				data[(x*4)   + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 100;
				data[(x*4)+1 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 250;
				data[(x*4)+2 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = y*2;
				data[(x*4)+3 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 255;
			}

			if(x > 50 && x < 70 && y > 0 && y < VOLUME_TEX_SIZE && z > 10 &&  z < 50)
			{
			
				data[(x*4)   + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 250;
				data[(x*4)+1 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 250;
				data[(x*4)+2 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = y*2;
				data[(x*4)+3 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 255;
			}

			if(x > 80 && x < 100 && y > 0 && y < VOLUME_TEX_SIZE && z > 10 &&  z < 50)
			{
			
				data[(x*4)   + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 250;
				data[(x*4)+1 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 70;
				data[(x*4)+2 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = y*2;
				data[(x*4)+3 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 255;
			}

			p =	vec3(x,y,z)- vec3(24,24,24);
			test = (length(p) < 40);
			//test = false;
			if(test)
				data[(x*4)+3 + (y * VOLUME_TEX_SIZE * 4) + (z * VOLUME_TEX_SIZE * VOLUME_TEX_SIZE * 4)] = 0;

		}}}

		Texture3D* t = new Texture3D(data, VOLUME_TEX_SIZE,VOLUME_TEX_SIZE,VOLUME_TEX_SIZE);
		delete data;
		return t;
	}

	void load3dBrain(Texture*& volume, Texture*& normals){
		auto brain = new GLushort[109 * 256 * 256];
		auto norms = new vec3[109 * 256 * 256];
		for (int i=0; i < 109; i++){
			string file=string("MRbrain/MRbrain.") + itos(i+1);
			fstream in(file, ios::in | ios::binary);
			if (!in.is_open()) cout << "can't open file: " << file << endl;
			in.read((char*)(brain+(i*256*256)),256*256*sizeof(GLushort));
			for (int j=i*256*256; j < i*256*256 + 256*256; j++)
				brain[j] = _byteswap_ushort(brain[j]);
				//cout << brain[j] << ", ";
		}
		volume = new Texture3D(brain,256,256,109,1,GL_RED,GL_UNSIGNED_SHORT);
		
#define i(x,y,z) (z)*256*256 + (y)*256 + (x)

		for (int z=1; z < 108; z++)
			for (int y=1; y < 255; y++)
				for (int x=1; x < 255; x++){
					vec3 n(
						brain[i(x-1,y,z)] - brain[i(x+1,y,z)]
						,brain[i(x,y-1,z)] - brain[i(x,y+1,z)]
						,brain[i(x,y,z-1)] - brain[i(x,y,z+1)]
					);

					if (_isnan(n.x) || _isnan(n.y) || _isnan(n.z))
						cout<<"tits"<<endl;

					//norms[i(x,y,z)] = (n.x==0&&n.y==0&&n.z==0) ? vec3(0) : normalize(n);
					
					norms[i(x,y,z)] = n == vec3(0) ?  
						normalize(vec3(x/255.f,y/255.f,z/108.f)-vec3(.5))
						: normalize(n);

					/*
					vec3 a = norms[i(x,y,z)];
					vec3 b = a*.5f + vec3(.5);
					vec3 c = b*2.f - vec3(1);
					if (abs(a.x - c.x) > .0000001){
						cout <<"-----------------"<<endl;
						cout <<a.x<<","<<a.y<<","<<a.z<<","<<endl;
						//cout <<b.x<<","<<b.y<<","<<b.z<<","<<endl;
						cout <<c.x<<","<<c.y<<","<<c.z<<","<<endl;
					}
					*/
				}
#undef i
		
		
		
		normals = new Texture3D(norms,256,256,109,GL_RGB32F,GL_RGB,GL_FLOAT);
		delete brain;
		delete norms;
		setWorldTransform(rotate(mat4(1),180.f,vec3(1,0,0)) * scale(mat4(1),vec3(1,1,.5)));
	}

	void setDelta(float delta){
		shader.enable();
		this->delta = std::max(delta,.001f);
	}

	void setAbsorbtion(float absorbtion){
		shader.enable();
		this->absorbtion = std::max(absorbtion, .1f);
	}

	void setThreshold(float threshold){
		shader.enable();
		this->threshold = std::max(threshold,.001f);
	}

	void setBackground(int i){
		background.set(backgrounds[i]);
	}

	void setWorldTransform(mat4 m){
		shader.enable();
		worldTransform = m;
		inverseTransform = inverse(m);
		normalTransform = mat3(transpose(inverse(m)));
	}

	void Scene::draw(){}

	void Viewport::draw(){
		frame();
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		background.draw();

		shader.enable();
		Shapes::cube()->draw();
	}

	void frame(){
		FPInput::frame();
		Scene::frame();

		if (keys[','])
			setDelta(*delta - .01*Clock::delta);
		if (keys['.'])
			setDelta(*delta + .01*Clock::delta);

		if (keys[';'])
			setAbsorbtion(*absorbtion - 5*Clock::delta);
		if (keys['\''])
			setAbsorbtion(*absorbtion + 5*Clock::delta);

		if (keys['k'])
			setThreshold(*threshold - .001*Clock::delta);
		if (keys['l'])
			setThreshold(*threshold + .001*Clock::delta);

		Clock::printFps(500);
	}

	//called by the ViewportManager
	virtual void resize(){
		Scene::resize(perspective(fovY, (GLfloat)w/h, nearPlane, farPlane), w, h);
	}

	virtual void mouseButton(int button, int state, int x, int y){
		FPInput::mouseButton(button, state, x, y);
	}

	virtual void mouseMove(int x, int y) {
		FPInput::mouseMove(x, y);

		//rotate worldTransform with right mouse
		float w = 5;
		if (mouseDown[1]){

			mat4 m = rotate(mat4(1), mouseDelta.x*w*Clock::delta, vec3(0,1,0))
				* rotate(mat4(1), -mouseDelta.y*w*Clock::delta, right());
			setWorldTransform(m * *worldTransform);
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

	~Volumez(){
		cout << "exiting" << endl;
	}

};


MyWindow win;
Volumez volumez(0,0,1,1);


void init(void)
{
	win();
	volumez();
	win.add(&volumez);
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
