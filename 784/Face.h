#include "MyWindow.h"
#include <limits.h>

const char* title = "Realistic Face";
/*orig
struct Blurrer : public Shader {
	Framebuffer fb1, fb2;
	Texture* primary;
	Texture tmp;

	Uniform2f scale;
	UniformSampler texture, stretch;

	void operator()(Texture* tex, Texture* stretchTex){
		Shader::operator()("face-blur.vert","face-blur.frag");
		primary = tex;
		tmp(primary->width,primary->height, primary->internalFormat);
		scale("scale",this);
		texture("tex", this, primary);
		stretch("stretch", this, stretchTex);
		fb1(primary);
		fb2(&tmp);
	}

	void blur(){
		Viewport::push(0,0,primary->width, primary->height);
		glDisable(GL_DEPTH_TEST);
		Shader::enable();
		scale = vec2(0,1.0f/primary->height);
		texture = primary;
		fb2.bind();
		Shapes::square()->draw();
		fb2.unbind();
		scale = vec2(1.0f/primary->width, 0);
		texture = &tmp;
		fb1.bind();
		Shapes::square()->draw();
		fb1.unbind();
		Viewport::pop();
		texture = primary;//leave texture unit as it was
	}
};
*/

struct Blurrer : public Shader {
	Texture* src;
	Texture tmp;
	Texture gauss[6];

	Uniform1f gaussWidth;
	Uniform2f scale;
	UniformSampler texture, stretch;

	void operator()(Texture* tex, Texture* stretchTex){
		Shader::operator()("face-blur.vert","face-blur.frag");
		src = tex;
		tmp(src->width,src->height, src->internalFormat, true);
		scale("scale",this);
		texture("tex", this, src);
		stretch("stretch", this, stretchTex);
		gaussWidth("gaussWidth", this);

		for (int i=0; i < 6; i++)
			gauss[i](src->width,src->height, src->internalFormat, true);//using mipmaps

	}

	void blur(int tex, float gw, bool feedback){
		gaussWidth = gw;
		scale = vec2(0, 1.0f/src->height);
		texture = feedback ? &gauss[tex] : src;
		tmp.bind2FB(false,false);
		Shapes::square()->draw();
		tmp.unbind2FB();

		scale = vec2(1.0f/src->width, 0);
		texture = &tmp;
		gauss[tex].bind2FB(false,false);
		Shapes::square()->draw();
		gauss[tex].unbind2FB();
	}

	void blur(bool feedback){
		Viewport::push(0,0,src->width, src->height);
		glDisable(GL_DEPTH_TEST);
		Shader::enable();
		blur(0, .0064, feedback);
		blur(1, .0484, feedback);
		blur(2, .187, feedback);
		blur(3, .567, feedback);
		blur(4, 1.99, feedback);
		blur(5, 7.41, feedback);
		Viewport::pop();
		texture = src;//leave texture unit as it was
	}
};


struct Face : Object {	
	Shader irradianceShader, beckmanShader, stretchShader;
	Texture *irradianceTex, *colorTex, *stretchTex, *perlinTex;
	Blurrer blurrer;
	int blur;

	UniformSampler irradianceNormals, shadowMap, normals, irradianceColor, color, irradiancePerlin, perlin, irradiance, specular, rho_d, beckman;
	UniformSampler gauss0, gauss1, gauss2, gauss3, gauss4, gauss5;
	Uniform1i reflectionsOn, phongOn;

	Face(mat4 transform, Texture* depth):
		Object(transform,"face.vert", "face.frag")
		,blur(1)
	{
		setVAO(Shapes::OBJ4("face/james_hi.obj"));//load vertex info

		//annoyingly I don't know any way to share sampler accross shaders...
		//setup final shader samplers
		Texture* normalsTex = new ILTexture("face/james_normal.png");
		colorTex = new ILTexture("face/james.png");
		//stretchTex = new ILTexture("face/skin_stretch.dds");
		perlinTex = new Perlin3D(128,1000);
		//perlinTex = new ILTexture("face/noise.png");
		normals("normals", this, normalsTex);
		color("colors", this, colorTex);
		specular("specular", this, new ILTexture("face/skin_spec.dds"));
		rho_d("rho_d", this, new ILTexture("face/rho_d.png"));
		perlin("perlin",this, perlinTex);
		reflectionsOn("reflectionsOn", this, 0);
		phongOn("phongOn", this, 0);

		//irradiance shader
		irradianceShader("face-irradiance.vert","face-irradiance.frag");//construct irradianceShader
		Object::addShader(&irradianceShader);//share worldTransform with normalTransform with irradianceShader
		//setup irradiance shader samplers
		irradianceNormals("normals",&irradianceShader,normalsTex);
		irradianceColor("colors", &irradianceShader,colorTex);
		irradiancePerlin("perlin",&irradianceShader,perlinTex);

		//make beckman tex
		beckman("beckman",this,new Texture(1024,1024,GL_RED,true,GL_CLAMP_TO_EDGE));//GL_CLAMP_TO_EDGE fixes an artifact when sampling close to edge
		beckmanShader("texturedSquare.vert", "beckman.frag");
		beckman.value->bind2FB();
		Shapes::square()->draw();
		beckman.value->unbind2FB();

		//make stretch tex
		stretchTex = new Texture(1024,1024,GL_RG,true,GL_CLAMP_TO_EDGE);//needs to be same res as irradianceTex
		stretchShader("face-stretch.vert", "face-stretch.frag");
		Object::addShader(&stretchShader);
		stretchTex->bind2FB();
		vao->draw();
		stretchTex->unbind2FB();

		//final shader irradiance
		irradianceTex = new Texture(1024,1024,GL_RGB,true);
		blurrer(irradianceTex, stretchTex);
		irradiance("irradiance",this,irradianceTex);

		//setup irradiance gauss samplers for final pass
		gauss0("gauss0", this, &blurrer.gauss[0]);
		gauss1("gauss1", this, &blurrer.gauss[1]);
		gauss2("gauss2", this, &blurrer.gauss[2]);
		gauss3("gauss3", this, &blurrer.gauss[3]);
		gauss4("gauss4", this, &blurrer.gauss[4]);
		gauss5("gauss5", this, &blurrer.gauss[5]);

		//shadows
		enableShadowCast();
		shadowMap("shadowMap",&irradianceShader,depth);
	}

	void render(){
		//draw irradiance
		irradianceTex->bind2FB();
		irradianceShader.enable();
		vao->draw();
		irradianceTex->unbind2FB();
		//irradianceTex->draw(0,0,200,200);
		
		for (int i=0; i < blur; i++)
			blurrer.blur(i>0);

		//irradianceTex->generateMipmaps();
		irradianceTex->draw(200,0,200,200);
		shadowMap.value->draw(400,0,200,200);
		specular.value->draw(600,0,200,200);
		rho_d.value->draw(800,0,200,200);
		stretchTex->draw(1000,0,200,200);
		beckman.value->draw(1200,0,200,200);

		for (int i=0; i < 6; i++)
			blurrer.gauss[i].draw(0,(5-i) * 200, 200, 200);

		//convert 

		//final draw
		draw();
		//debug the irradiance texture
	}

	void toggleReflections(){
		Shader::enable();
		reflectionsOn = *reflectionsOn ? 0 : 1;
	}
	void togglePhong(){
		Shader::enable();
		phongOn = *phongOn ? 0 : 1;
	}
};

struct FaceScene : public Viewport, public Scene, public FPInput {
	Light light;
	Face face;

	UniformSampler reflections;
	CubeBackground background;
	CubeMap* backgrounds[3];

	float nearPlane, farPlane, fovY;

	FaceScene(float x, float y, float w, float h):
		Viewport(x,y,w,h)
		,nearPlane(.1f), farPlane(1001.f), fovY(60.f)
		,FPInput(15)
		,light(vec3(1,1,1), 2048, perspective(33.f, 1.f, 40.f, 85.f), glm::lookAt(vec3(0,0,60),vec3(0,0,0),vec3(0,1,0)))
		,face(mat4(1), &light.depth)
	{
		Scene::operator()(this);
		Viewer::pos = vec3(0,0,20.f);
		addLight(&light);

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

		reflections("reflections",&face,backgrounds[2]);
		background(backgrounds[2]);

	}

	void setBackground(int i){
		background.set(backgrounds[i]);
	}

	void Scene::draw(){}

	void Viewport::draw(){
		FPInput::frame();
		Scene::frame();
		frame();
		Clock::printFps(500);

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		background.draw();

		drawShadows();

		face.render();
	}

	void drawShadows(){
		light.bind();
		face.shadowDraw();
		light.unbind();
	}

	void frame(){
		static int counter = 0;
		if (counter++ % 10 == 0){
			if (keys[','])
				face.blur = std::max(1, face.blur-1);
			if (keys['.'])
				face.blur++;
		}
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
			face.setWorldTransform(m * face.getWorldTransform());
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
			case ',':
				face.blur = std::max(1, face.blur-1);
				break;
			case '.':
				face.blur++;
				break;
			case 'r':
				face.toggleReflections();
				break;
			case ' ':
				face.togglePhong();
				break;
			case 'p':
				face.setWorldTransform(mat4(1));
		}
	}

	virtual void keyUp(unsigned char key, int x, int y){
		FPInput::keyUp(key, x, y);
	}
};


MyWindow win;
FaceScene* face=NULL;

void init(void)
{
	win();
	face = new FaceScene(0,0,1,1);
	win.add(face);
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
	glutPostRedisplay();
}

void reshape(int w, int h)
{
	assert(face);
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
