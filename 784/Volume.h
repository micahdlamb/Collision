#include "MyWindow.h"
//#include "intrin.h"
#include <limits.h>

const char* title = "Micah Lamb's Volumez";

class Volume {
public:
	typedef detail::tvec3<uchar> ucvec3;
	U x, y, z;
	uchar* vals;
	vec3* gradients;
	ucvec3 colors[256];

	Volume(string vol, U x, U y, U z):x(x),y(y),z(z){
		vals = new uchar[x*y*z];
		gradients = new vec3[x*y*z];//hopefully default constuctor initializes to vec3(0)

		fstream in(vol, ios::in | ios::binary);
		if (!in.is_open()) error(string("can't open file: ") + vol);
		in.read((char*)vals, x*y*z*sizeof(uchar));

		//calc gradients
		for (U i=1; i < x-1; i++)
			for (U j=1; j < y-1; j++)
				for (U k=1; k < z-1; k++){
					gradient(i,j,k) = vec3(
						val(i+1,j,k)/255.f - val(i-1,j,k)/255.f
						,val(i,j+1,k)/255.f - val(i,j-1,k)/255.f
						,val(i,j,k+1)/255.f - val(i,j,k-1)/255.f
					);
				}
	}

	void loadColors(string file){
		ifstream fin(file);
		if (fin.fail())
			error(string("unable to open off file: ") + file);
		stringstream ss;
		string line;
		int index;
		uvec3 v;
		while (getline(fin, line)){
			ss.str("");
			ss.clear();
			ss << line;
			ss >> index;
			ss >> v.x;
			ss >> v.y;
			ss >> v.z;
			colors[index] = v;
		}
	}

	U index(U i, U j, U k){
		//return i*y*z + j*z + k;
		return k*x*y + i*x + j;
	}

	uchar& val(U i, U j, U k){
		return vals[index(i,j,k)];
	}
	
	vec3& gradient(U i, U j, U k){
		return gradients[index(i,j,k)];
	}

	~Volume(){
		delete [] vals;
		delete [] gradients;
	}
};

struct VolumeRenderer : public Viewport, public Scene, public FPInput {
	Shader shader;
	UniformMat4 worldTransform;
	UniformMat4 inverseTransform;
	UniformMat3 normalTransform;
	Uniform1f delta;
	Uniform1f absorbtion;
	Uniform1f threshold;
	Uniform1f density_gradient_mix;
	Uniform1f brightness;

	UniformSampler densities;
	UniformSampler gradients;
	UniformSampler colors;

	CubeBackground background;
	CubeMap* backgrounds[3];

	float nearPlane, farPlane, fovY;

	VolumeRenderer(float x, float y, float w, float h):
		Viewport(x,y,w,h)
		,nearPlane(.1f), farPlane(1001.f), fovY(60.f)
	{}

	void operator()(){
		Scene::operator()(this);
		//shader("volume.vert", "volume.frag");
		shader("volume.vert", "volume.frag");
		worldTransform("worldTransform",&shader);
		inverseTransform("inverseTransform",&shader);
		normalTransform("normalTransform",&shader);
		delta("delta",&shader,.005);
		absorbtion("absorbtion",&shader,20);
		threshold("threshold",&shader,.015);
		density_gradient_mix("density_gradient_mix",&shader, 0);
		brightness("brightness",&shader,2);

		Scene::globals.lights[0].color = vec3(1,1,1);
		Scene::globals.lights[0].pos = vec3(0,0,25);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);

		//volume("volume",&shader,gen3dTexture());
		Texture *d, *g, *c;
		load(d,g,c);
		densities("densities",&shader,d);
		gradients("gradients",&shader,g);
		colors("colors",&shader,c);

		Viewer::pos = vec3(0,0,1.f);
		show_chest();

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

	Texture *bonsai_densities, *bonsai_gradients, *bonsai_colors;
	Texture *chest_densities, *chest_gradients, *chest_colors;

	void load(Texture*& densities, Texture*& gradients, Texture*& colors){
		Volume bonsai("volumes/bonsai.vol", 256, 256, 128);
		bonsai.loadColors("volumes/bonsai.colors");
		bonsai_densities = new Texture3D(bonsai.vals,bonsai.x,bonsai.y,bonsai.z,1,GL_RED,GL_UNSIGNED_BYTE);
		bonsai_gradients = new Texture3D(bonsai.gradients,bonsai.x,bonsai.y,bonsai.z,GL_RGB32F,GL_RGB,GL_FLOAT);
		bonsai_colors = new Texture1D(bonsai.colors, 256, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);

		Volume chest("volumes/chest.vol", 256, 256, 279);
		chest.loadColors("volumes/chest.colors");
		densities = chest_densities = new Texture3D(chest.vals,chest.x,chest.y,chest.z,1,GL_RED,GL_UNSIGNED_BYTE);
		gradients = chest_gradients = new Texture3D(chest.gradients,chest.x,chest.y,chest.z,GL_RGB32F,GL_RGB,GL_FLOAT);
		colors = chest_colors = new Texture1D(chest.colors, 256, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
		setWorldTransform(scale(mat4(1), vec3(1, 1, 279.f/256)));
		printGLErrors("after color");
		
		
		/*
		//Volume vol("volumes/bonsai.vol", 256, 256, 128);
		//vol.loadColors("volumes/bonsai.colors");
		Volume vol("volumes/chest.vol", 256, 256, 279);
		vol.loadColors("volumes/chest.colors");
		setWorldTransform(scale(mat4(1), vec3(1, 1, 279.f/256)));

		densities = new Texture3D(vol.vals,vol.x,vol.y,vol.z,1,GL_RED,GL_FLOAT);
		gradients = new Texture3D(vol.gradients,vol.x,vol.y,vol.z,GL_RGB32F,GL_RGB,GL_FLOAT);
		colors = new Texture1D(vol.colors, 256, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE);
		*/
	}

	void show_bonsai(){
		densities = bonsai_densities;
		gradients = bonsai_gradients;
		colors = bonsai_colors;
		threshold = .19f;
		absorbtion = 60.f;
		density_gradient_mix = .5f;
		brightness = 2.f;
		setWorldTransform(scale(rotate(mat4(1), 180.f, vec3(1,0,0)), vec3(1, 1, 1)));
	}

	void show_chest(){
		densities = chest_densities;
		gradients = chest_gradients;
		colors = chest_colors;
		threshold = .015f;
		absorbtion = 25.f;
		density_gradient_mix = .1f;
		brightness = 3.5f;
		
		setWorldTransform(scale(rotate(mat4(1), -90.f, vec3(1,0,0)), vec3(1, 1, 279.f/256)));
	}

	void setDelta(float delta){
		shader.enable();
		this->delta = std::max(delta,.0005f);
	}

	void setAbsorbtion(float absorbtion){
		shader.enable();
		this->absorbtion = std::max(absorbtion, .1f);
	}

	void setThreshold(float threshold){
		shader.enable();
		this->threshold = std::max(threshold,0.f);
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
			setAbsorbtion(*absorbtion - 10*Clock::delta);
		if (keys['\''])
			setAbsorbtion(*absorbtion + 10*Clock::delta);

		if (keys['k'])
			setThreshold(*threshold - .15*Clock::delta);
		if (keys['l'])
			setThreshold(*threshold + .15*Clock::delta);

		if (keys['n'])
			density_gradient_mix = glm::max(0.f, *density_gradient_mix - .5f*Clock::delta);
		if (keys['m'])
			density_gradient_mix = glm::min(1.f, *density_gradient_mix + .5f*Clock::delta);
		if (keys['['])
			brightness = std::max(.5f, *brightness - .25f*Clock::delta);
		if (keys[']'])
			brightness = std::min(4.f, *brightness + .25f*Clock::delta);

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
			case '9':
				show_bonsai();
				break;
			case '0':
				show_chest();
				break;
		}
	}

	virtual void keyUp(unsigned char key, int x, int y){
		FPInput::keyUp(key, x, y);
	}

	~VolumeRenderer(){
		cout << "exiting" << endl;
	}

};


MyWindow win;
VolumeRenderer vol(0,0,1,1);


void init(void)
{
	win();
	vol();
	win.add(&vol);
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
