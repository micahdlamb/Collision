typedef GLuint I;
GLenum GL_I = GL_UNSIGNED_INT;
#include "globals.h"
#include "Bezier.h"
#include "BSpline.h"
#include "Surfaces.h"
#include "ControlCurve.h"
#include "Smoother.h"
#include "Surface.h"
#include "MyWindow.h"

const char* title = "Micah Lamb's 784 lab 3";

MyWindow win;
ControlCurve *cc1, *cc2;
Surface *surface;

#include "gui.h"

void clear(){
	cc1->clear();
	cc2->clear();
	surface->clear();
	glutPostRedisplay();
}

void computeSurface(){
	surface->buildFromCurves();
}

void compute(){
	cc1->compute();
	cc2->compute();
	computeSurface();
	glutPostRedisplay();
}

void init(void)
{
	win();
	cc1 = new ControlCurve(0,0,.35,.5,vec3(1,.95,1));
	cc2 = new ControlCurve(0,.5,.35,.5,vec3(.9,1,.9));
	surface = new Surface(.35,0,.65,1,cc1, cc2);
	win.add(cc1);
	win.add(cc2);
	win.add(surface);
}

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
