void clear();
void compute();

//GUI Stuff ******************************************
GLUI *glui;
const int MAXITERATIONS = 8;
const int MAXSLICES = 100;

//live vars
int pattern=1;
int background=0;

char load_file[sizeof(GLUI_String)] = "";
char save_file[sizeof(GLUI_String)] = "";



//control ids
enum Controls {
	QUIT
	,CURVETYPE
	,ITERATIONS
	,SURFACETYPE
	,SLICES
	,EXTRUDE
	,SPATTERN
	,SBACKGROUND
	//buttons
	,CLEAR
	,SAVE
	,CUBE
	,PYRAMID
	,SPRING
	,TERRAIN
	,LOAD_FILE
	,SAVE_FILE
	//curve surface smoothers
	,BEZIER_SURFACE
	,CUBIC_B_SPLINE_SURFACE

	//smoothers
	,CATMULL_CLARK
	,DOO_SABIN
	,LOOP
};

void onChange(int id){
	switch (id){
		case CURVETYPE:
			cout << "curveType changed to " << curveType << endl;
			compute();
			break;
		case ITERATIONS:
			cout << "iteration changed to " << iterations << endl;
			compute();
			break;
		case SURFACETYPE:
			cout << "surfaceType changed to " << surfaceType << endl;
			compute();
			break;
		case SLICES:
			cout << "slices changed to " << slices << endl;
			compute();
			break;
		case EXTRUDE:
			cout << "extrusion changed to " << extrude << endl;
			compute();
			break;
		case SPATTERN:
			cout <<"pattern change to " << pattern << endl;
			surface->setPattern(pattern);
			break;
		case SBACKGROUND:
			cout <<"background changed to " << background << endl;
			surface->setBackground(background);
			break;
	}

	//using idle draw loop to redraw for now
}

//call backs
void quit(int id){
	exit(0);
}

void onButton(int id){
	switch (id){
		case CLEAR:
			clear();
			break;
		case SAVE:{
			string s = save_file;
			if (s.size())
				surface->print(s.c_str());
			break;
		}
		case CUBE:
			surface->cube();
			break;
		case PYRAMID:
			surface->pyramid();
			break;
		case SPRING:
			surface->spring();
			break;
		case TERRAIN:
			surface->terrain();
			break;
		case LOAD_FILE:{
			string s = load_file;
			if (s.size() > 0)
				surface->load(s.c_str());
			break;
		}
		case BEZIER_SURFACE:
			surface->surface(0);
			break;
		case CUBIC_B_SPLINE_SURFACE:
			surface->surface(1);
			break;
		case CATMULL_CLARK:
			surface->smooth(0);
			break;
		case DOO_SABIN:
			surface->smooth(1);
			break;
		case LOOP:
			surface->smooth(2);
			break;
	}
	glutPostRedisplay();
}

void init_glui(){
	//glui = GLUI_Master.create_glui( "Options");
	glui = GLUI_Master.create_glui_subwindow(main_window,GLUI_SUBWINDOW_RIGHT );
	glui->set_main_gfx_window(main_window);

	//add curve options
	GLUI_Panel *curvePanel = glui->add_panel ( "Curve Type" );
	//curve type
	GLUI_RadioGroup *curveType_rg = glui->add_radiogroup_to_panel(curvePanel,&curveType,CURVETYPE,onChange);
	glui->add_radiobutton_to_group( curveType_rg, "Bezier" );
	glui->add_radiobutton_to_group( curveType_rg, "Quadratic B-spline" );
	glui->add_radiobutton_to_group( curveType_rg, "Closed Quadratic B-spline" );
	glui->add_radiobutton_to_group( curveType_rg, "Cubic B-spline" );
	glui->add_radiobutton_to_group( curveType_rg, "Closed Cubic B-spline" );
	//iterations
	GLUI_Spinner *iterations_spinner = glui->add_spinner_to_panel(curvePanel, "iterations", GLUI_SPINNER_INT, &iterations, ITERATIONS, onChange);
	iterations_spinner->set_int_limits(0,MAXITERATIONS,GLUI_LIMIT_CLAMP);


	//add surface options
	GLUI_Panel *surfacePanel = glui->add_panel ( "Surface Type" );
	//curve type
	GLUI_RadioGroup *surfaceType_rg = glui->add_radiogroup_to_panel(surfacePanel,&surfaceType,SURFACETYPE,onChange);
	glui->add_radiobutton_to_group( surfaceType_rg, "Revolution" );
	glui->add_radiobutton_to_group( surfaceType_rg, "Extrusion" );
	glui->add_radiobutton_to_group( surfaceType_rg, "Sweep" );
	//slices
	GLUI_Spinner *slices_spinner = glui->add_spinner_to_panel(surfacePanel, "slices", GLUI_SPINNER_INT, &slices, SLICES, onChange);
	slices_spinner->set_int_limits(2,MAXSLICES,GLUI_LIMIT_CLAMP);
	//extrusion
	GLUI_Spinner *extrusion_spinner = glui->add_spinner_to_panel(surfacePanel, "extrusion", GLUI_SPINNER_FLOAT, &extrude, EXTRUSION, onChange);
	extrusion_spinner->set_float_limits(.0001f,5,GLUI_LIMIT_CLAMP);

	//add pattern options
	GLUI_Panel *patternPanel = glui->add_panel ( "Pattern" );
	GLUI_RadioGroup *patternType_rg = glui->add_radiogroup_to_panel(patternPanel,&pattern,SPATTERN,onChange);
	glui->add_radiobutton_to_group( patternType_rg, "Purple" );
	glui->add_radiobutton_to_group( patternType_rg, "Colorz" );
	glui->add_radiobutton_to_group( patternType_rg, "Stripes" );
	//glui->add_radiobutton_to_group( patternType_rg, "World Colorz" );

	//add pattern options
	GLUI_Panel *backgroundPanel = glui->add_panel ( "Background" );
	GLUI_RadioGroup *background_rg = glui->add_radiogroup_to_panel(backgroundPanel,&background,SBACKGROUND,onChange);
	glui->add_radiobutton_to_group( background_rg, "Clouds" );
	glui->add_radiobutton_to_group( background_rg, "Morning" );
	glui->add_radiobutton_to_group( background_rg, "Hills" );
	glui->add_radiobutton_to_group( background_rg, "Bright Day" );

	GLUI_Panel *shapesPanel = glui->add_panel ( "Shapes" );
	glui->add_button_to_panel( shapesPanel, "Cube", CUBE, onButton );
	glui->add_button_to_panel( shapesPanel, "Pyramid", PYRAMID, onButton );
	glui->add_button_to_panel( shapesPanel, "Spring", SPRING, onButton );
	glui->add_button_to_panel( shapesPanel, "Terrain", TERRAIN, onButton );
	glui->add_button_to_panel( shapesPanel, "Load File", LOAD_FILE, onButton );
	glui->add_edittext_to_panel(shapesPanel, "", GLUI_EDITTEXT_TEXT, load_file);

	GLUI_Panel *smoothSurfacePanel = glui->add_panel ( "Surface" );
	glui->add_button_to_panel( smoothSurfacePanel, "Bezier", BEZIER_SURFACE, onButton );
	glui->add_button_to_panel( smoothSurfacePanel, "Cubic B-spline", CUBIC_B_SPLINE_SURFACE, onButton );

	GLUI_Panel *smootherPanel = glui->add_panel ( "Smoothers" );
	glui->add_button_to_panel( smootherPanel, "Catmull-Clark", CATMULL_CLARK, onButton );
	glui->add_button_to_panel( smootherPanel, "Doo-Sabin", DOO_SABIN, onButton );
	glui->add_button_to_panel( smootherPanel, "Loop", LOOP, onButton );

	glui->add_button( "Save to File", SAVE, onButton );
	glui->add_edittext("", GLUI_EDITTEXT_TEXT, save_file);
	glui->add_button( "Clear", CLEAR, onButton );
	glui->bkgd_color = RGBc(200,255,100);
}
