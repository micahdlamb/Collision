#include "includes.h"

int main_window;
uvec2 dim(1600,900);
#define NUM_BUFFERS GLUT_DOUBLE

//#include "lab3.h"
//#include "Volume.h"
//#include "Tessellation.h"
//#include "Tessellation2.h"
//#include "RayTracer.h"
#include "GlassBalls.h"
//#include "Bullet.h"
//#include "Lens.h"
//#include "Face.h"

//outdated
//#include "cubez.h"
//#include "Fireworks.h"
//#include "Picking.h"

int main (int argc, char* argv[])
{
	glutInit(&argc, argv);
	glutInitWindowSize(dim.x,dim.y);
	glutInitDisplayMode(GLUT_RGBA|GLUT_ALPHA|NUM_BUFFERS|GLUT_DEPTH);
	main_window = glutCreateWindow(title);

	/*
	//causes glBindBufferBase to segfault no clue why
	glutInitContextVersion(4, 2);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	*/

	//will improve error handling later
	try {
		GLState::init();
		ilInit();
		iluInit();

		GLenum err = glewInit();
		if (GLEW_OK != err)
			error("glewInit failed, aborting.");

		cout << "Using GLEW " << glewGetString(GLEW_VERSION) << endl;
		cout << "OpenGL version " << glGetString(GL_VERSION) << " supported" << endl;

		srand(time(NULL));
	
		init_glui();
		init();

		glutMotionFunc(mouseMoved);
		glutPassiveMotionFunc(passiveMouseMoved);
		glutIgnoreKeyRepeat(true);
	
		glutKeyboardUpFunc(keyUp);
		glutDisplayFunc(display);

		//glutKeyboardFunc(keyDown);
		//glutMouseFunc(mouseButton);
		//glutReshapeFunc(reshape);

		GLUI_Master.set_glutKeyboardFunc( keyDown );
		GLUI_Master.set_glutSpecialFunc( NULL );
		GLUI_Master.set_glutMouseFunc( mouseButton );
		GLUI_Master.set_glutReshapeFunc( reshape );

		GLUI_Master.set_glutIdleFunc(idle);
		//glutIdleFunc(idle);

		glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION);//stop segfault on close somehow
		glutMainLoop();
	} catch (string e){
		cout << e << endl;
		pause();
	}

	return 0;
}
