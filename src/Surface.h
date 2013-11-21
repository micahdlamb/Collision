#pragma once
//I, the index type must be defined
struct Surface : public Viewport, public Scene, public FPInput {
	//typedef GLushort I;
	typedef glm::detail::tvec3<I> I3;

	Shader shader;
	UniformMat4 worldTransform;
	UniformMat3 normalTransform;
	Uniform1i pattern;
	
	Uniform1i reflectionsOn;
	Uniform1i blur;
	UniformSampler reflections;
	CubeBackground background;
	CubeMap* backgrounds[4];
	bool wireframe;
	int terrainFile;

	float nearPlane, farPlane, fovY;
	const static int MAXVERTICES = 1000000;//1 million dollars

	ControlCurve* cc1, *cc2;

	Smoother<I> smoother;

	enum Mode {NOTHING,TRIANGLES,QUADS,CURVE,CUBE,PYRAMID,SPRING,TERRAIN,CATMULL,DOO_SABIN,LOOP};

	Mode mode;
	vector<vec3> vertices, normals;
	vector<I> indices;
	VAO geom;

	Surface(float x, float y, float w, float h, ControlCurve* cc1, ControlCurve* cc2):
		Viewport(x,y,w,h)
		,nearPlane(.01f), farPlane(1001.f), fovY(60.f)
		,cc1(cc1), cc2(cc2)
		,wireframe(false)
		,terrainFile(-1)
		,mode(NOTHING)
	{
		Scene::operator()(this);
		shader("surface.vert", "surface.frag");
		shader.enable();
		worldTransform("worldTransform",&shader);
		normalTransform("normalTransform",&shader);
		pattern("pattern",&shader, 1);
		Scene::globals.lights[0].color = vec3(0,1,0);
		Scene::globals.lights[0].pos = vec3(0,0,25);
		reflectionsOn("reflectionsOn",&shader,1);
		blur("blur",&shader,0);

		Viewer::pos = vec3(0,0,1.5f);

		#define STR(x) #x
		#define CM(file) STR(cubemaps/clouds/##file)
		char* clouds[] = {CM(px.png),CM(nx.png),CM(py.png),CM(ny.png),CM(pz.png),CM(nz.png)};
		backgrounds[0] = new CubeMap(clouds, IL_ORIGIN_UPPER_LEFT);
		#undef CM
		#define CM(file) STR(cubemaps/deadmeat/##file)
		char* deadmeat[] = {CM(px.jpg),CM(nx.jpg),CM(py.jpg),CM(ny.jpg),CM(pz.jpg),CM(nz.jpg)};
		backgrounds[1] = new CubeMap(deadmeat, IL_ORIGIN_UPPER_LEFT);
		#undef CM
		#define CM(file) STR(cubemaps/hills/##file)
		char* hills[] = {CM(px.png),CM(nx.png),CM(py.png),CM(ny.png),CM(pz.png),CM(nz.png)};
		backgrounds[2] = new CubeMap(hills, IL_ORIGIN_UPPER_LEFT);
		#undef CM
		#define CM(file) STR(cubemaps/brightday/##file)
		char* brightday[] = {CM(px.png),CM(nx.png),CM(py.png),CM(ny.png),CM(pz.png),CM(nz.png)};
		backgrounds[3] = new CubeMap(brightday, IL_ORIGIN_UPPER_LEFT);

		reflections("reflections",&shader,backgrounds[0]);
		background(backgrounds[0]);

		geom(mode, GL_DYNAMIC_DRAW);
		geom.bind();
		geom.buffer(NULL, MAXVERTICES * sizeof(vec3));//vertices
		geom.in(3,GL_FLOAT);
		geom.buffer(NULL, MAXVERTICES * sizeof(vec3));//normals
		geom.in(3,GL_FLOAT);
		geom.buffer(NULL, MAXVERTICES * sizeof(GL_I) * 4);//*4 is guess, think about it later
		geom.indices(GL_I);
		geom.unbind();
	}

	void setGeometry(vector<vec3>& vertices, vector<I>& indices, Mode mode){
		this->vertices = vertices;
		this->indices = indices;
		setGeometry(mode);
	}

	//for some funcs its easier to copy verts and indices in directly
	void setGeometry(Mode mode){
		setMode(mode);
		normals = computeNormals<I>(vertices, indices, faceSize());
		push();
	}

	void setMode(Mode mode){
		if (this->mode != mode)
			onModeChange(this->mode, mode);
		this->mode = mode;
	}

	void onModeChange(Mode from, Mode to){
		if (from == DOO_SABIN)//accomodate hack for doo_sabin
			smoother.feedbackAvailable = false;
	}


	void push(){
		if (!indices.size()) return;
		geom.update(&vertices[0], vertices.size()*sizeof(vec3), 0);
		geom.update(&normals[0], normals.size() * sizeof(vec3), 1);
		geom.update(&indices[0], indices.size() * sizeof(I), 2);
	}

	GLenum glMode(){
		switch(mode){
			case DOO_SABIN:
			case LOOP:
			case PYRAMID:
			case TRIANGLES:
				return GL_TRIANGLES;
			default:
				return GL_QUADS;
		}
	}

	int faceSize(){
		return glMode()==GL_QUADS?4:3;
	}

	void clear(){
		setMode(NOTHING);
		indices.clear();
		vertices.clear();
	}

	void setPattern(int pattern){
		shader.enable();

		switch(pattern){
			case 0:
				Scene::globals.lights[0].color = vec3(0,1,0);
				break;
			case 1:
				Scene::globals.lights[0].color = vec3(1,1,0);
				break;
			case 2:
				Scene::globals.lights[0].color = vec3(0,0,1);
				break;
			case 3:
				Scene::globals.lights[0].color = vec3(1,1,0);
				break;
		}

		this->pattern = pattern;
	}

	void toggleReflections(){
		shader.enable();
		reflectionsOn = *reflectionsOn ? 0 : 1;
	}

	void setBlur(int i){
		shader.enable();
		blur = clamp(i,0,8);
	}

	void setBackground(int i){
		shader.enable();
		//reflections = backgrounds[i];
		background.set(backgrounds[i]);
	}

	void setWorldTransform(mat4& m){
		shader.enable();
		worldTransform = m;
		normalTransform = mat3(transpose(inverse(m)));
	}

	virtual void draw(){
		//glEnable(GL_CULL_FACE);
		frame();
		
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		background.draw();

		shader.enable();

		glPushAttrib(GL_POLYGON_BIT);
		if (wireframe) glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

		geom.draw(indices.size(),0,glMode());
		glPopAttrib();
	}

	///////////////////////
	//CURVE & SURFACE STUFF
	///////////////////////
	I strideX;

	bool wrapX(){
		return surfaceType == REVOLUTION || (curveClosed() && surfaceType == SWEEP);
	}

	bool wrapY(){
		return curveClosed();
	}

	void buildFromCurves(){
		clear();
		if (!((surfaceType == REVOLUTION || surfaceType == EXTRUSION) && cc1->curvePts.size() > 2
			|| cc1->curvePts.size() > 2 && cc2->curvePts.size() > 1))
			return;

		vector<vec2> curve1 = cc1->getCurvePtPositions(), curve2 = cc2->getCurvePtPositions();
		//remove duplicate point on closed curves
		if (curveClosed()){
			curve1.pop_back();
			if (curve2.size())
				curve2.pop_back();
		}
	
		//compute vertices
		switch (surfaceType){
			case REVOLUTION:
				vertices = Surfaces::revolution(curve1);
				break;
			case EXTRUSION:
				vertices = Surfaces::extrusion(curve1);
				break;
			case SWEEP:
				vertices = Surfaces::sweep(curve1, curve2, extrude);
				break;
		}

		strideX = (I)curve1.size();//store strideX for use in other surface funcs
		indices = gridIndices((I)vertices.size(), strideX, wrapX(), wrapY());
		setGeometry(CURVE);
	}

	//convert vertices to grid (2d array vector)
	vector<vector<vec3 >> unflatten(){
		vector<vector<vec3 >> grid(vertices.size() / strideX);
		for (U i=0,x=0; i < vertices.size(); i+=strideX,x++){
			grid[x].resize(strideX);
			for (U y=0; y < strideX; y++)
				grid[x][y] = vertices[i+y];
		}
		return grid;
	}

	//convert grid to 
	void flatten(vector<vector<vec3 >> grid, bool wrapX, bool wrapY){
		vertices.clear();
		strideX = grid[0].size();
		for (U x=0; x < grid.size(); x++){
			//if (wrapX && x == grid.size()-1)
				//continue;
			for (U y=0; y < strideX; y++)
				vertices.push_back(grid[x][y]);
		}
		indices = gridIndices((I)vertices.size(), strideX, wrapX, wrapY);
	}

	void surface(int type){
		if (mode != CURVE) return;
		//construct grid
		auto grid = unflatten();
		switch (type){
			case 0:
				if (strideX > 25 || (surfaceType == REVOLUTION && slices > 25)){
					cout << "too many curve points or slices.... will take too long" << endl;
					return;
				}

				grid = Bezier::casteljauSurf(grid, 100, wrapX(), wrapY());
				flatten(grid, wrapX(), wrapY());
				break;
			case 1:
				grid = BSpline::cubic_surface(grid, 3, wrapX(), wrapY());
				flatten(grid, wrapX(), wrapY());
				break;
		}
		
		setGeometry(CURVE);
	}


	void smooth(int type=0){
		if (!indices.size()) return;
		Mode mode;
		switch (type){
			case 0:
				mode = CATMULL;
				smoother.catmull_clark(vertices, indices, faceSize());
				break;
			case 1:
				mode = DOO_SABIN;
				smoother.doo_sabin(vertices, indices, faceSize());
				break;
			case 2:
				mode = LOOP;
				smoother.loop(vertices, indices, faceSize());
		}

		smoother.printErrors();
		setGeometry(smoother.newVertices, smoother.newIndices, mode);
		cout << "smooth complete" << endl;
	}


	void cube(){
		GLfloat v[] = {
		  -.5,  .5,  .5,
		  -.5, -.5,  .5,
		   .5, -.5,  .5,
		   .5,  .5,  .5,
		  -.5,  .5, -.5,
		  -.5, -.5, -.5,
		   .5, -.5, -.5,
		   .5,  .5, -.5,
		};

		I i[] = {
		  0, 1, 2, 3,
		  3, 2, 6, 7,
		  7, 6, 5, 4,
		  4, 5, 1, 0,
		  0, 3, 7, 4,
		  1, 5, 6, 2
		};

		vertices.assign((vec3*)v, (vec3*)(v + 24));//strange that casts required
		indices.assign(i, i + 24);
		setGeometry(CUBE);
	}

	void pyramid(){
		GLfloat v[] = {
			-.5,-.5,.5,
			.5,-.5,.5,
			.5,-.5,-.5,
			-.5,-.5,-.5,
			0, .5, 0
		};

		I i[] = {
			0,3,1,
			1,3,2,
			0,1,4,
			1,2,4,
			2,3,4,
			3,0,4,
		};

		vertices.assign((vec3*)v, (vec3*)(v + 15));//strange that casts required
		indices.assign(i, i + 18);
		setGeometry(PYRAMID);
	}


	vector<vec2> circlePts(float radius, int numPts){
		vector<vec2> pts;
		float dtheta = 2*PI / numPts;
		float EPS = dtheta / 2;
		for (float theta=0; theta < 2*PI - EPS; theta+=dtheta)
			pts.push_back(vec2(cos(theta), sin(theta)) * radius);
		return pts;
	}

	vector<vec3> helixPts(float loops, float radius, float height, int numPts){
		vector<vec3> pts;
		numPts--;//since loop adds extra point
        float rads = 2*PI * loops;
		float dtheta = rads / numPts;
		float dy = height / numPts;
        float EPS = dtheta / 2;
		for (float theta=0, y=-height/2; abs(theta) < abs(rads)+EPS; theta+=dtheta, y+=dy)
			pts.push_back(vec3(cos(theta) * radius, y, -sin(theta) * radius));
		return pts;
	}

    void triangulateQuads(vector<I>& indices){
        vector<I> triIndices;
        for (size_t i=0; i < indices.size(); i+=4){
            triIndices.push_back(indices[i]);
            triIndices.push_back(indices[i+1]);
            triIndices.push_back(indices[i+3]);
            triIndices.push_back(indices[i+1]);
            triIndices.push_back(indices[i+2]);
            triIndices.push_back(indices[i+3]);
        }
        indices = triIndices;
    }

	//fill in a shape with triangles
	template <class I>
	vector<I> triangulate(I begin, I end, I pt, bool reverse=false){
		vector<I> indices;
		for (auto i=begin; i < end; i++){
			indices.push_back(i);
			indices.push_back((i == end-1) ? begin : i+1);
			indices.push_back(pt);
		}
		if (reverse)
			std::reverse(indices.begin(), indices.end());
		return indices;
	}

	//must have even number of points >= 6
	template <class I>
	vector<I> quadFan(I begin, I end, I center, bool reverse=false){
		vector<I> indices;
		for (I i = begin; i < end; i += 2){
			indices.push_back(i);
			indices.push_back(i+1);
			indices.push_back((i+2 == end) ? begin : i+2);
			indices.push_back(center);
		}
		if (reverse)
			for (size_t i=0; i < indices.size(); i+=4)
				swap(indices[i], indices[i+2]);
		return indices;
	}

	void spring(float loops=5, float radius=.1, float thickness=.05, float height=1, int verts=2000, bool smoothCaps=false){
		float helixLen = sqrt(pow(loops * PI * radius * 2, 2) + pow(height, 2));
		float circleLen = PI * thickness * 2;
		float ratio = circleLen / helixLen;
		int helixVerts = sqrt(verts / ratio);
		int circleVerts = verts / helixVerts;
		//make sure circleVerts >= 6 and even so the quadFan func will work
		if (circleVerts < 6) circleVerts = 6;
		if (circleVerts % 2) circleVerts++;

		//generate helix quads
		auto circle = circlePts(thickness, circleVerts);
		auto helix = helixPts(loops, radius, height, helixVerts);
		vertices = Surfaces::sweep(circle, helix);
		strideX = (I)circle.size();//store strideX for use in other smoothing funcs
		indices = gridIndices((I)vertices.size(), strideX, false, true);

		//generate caps quads
		if (smoothCaps){
            auto cap1 = quadFan<I>(vertices.size()-circle.size(), vertices.size(), vertices.size(), true);
		    vertices.push_back(helix.back());
		    auto cap2 = quadFan<I>(0, circle.size(), vertices.size());
		    vertices.push_back(helix[0]);
		    indices.insert(indices.end(), cap1.begin(), cap1.end());
		    indices.insert(indices.end(), cap2.begin(), cap2.end());
		}
        else {
            auto helixEnd = vertices.size();
            vertices.insert(vertices.end(), vertices.begin(), vertices.begin() + circle.size());
            auto cap1 = quadFan<I>(vertices.size()-circle.size(), vertices.size(), vertices.size());
            vertices.push_back(helix[0]);

            vertices.insert(vertices.end(), &vertices[helixEnd - circle.size()], &vertices[helixEnd]);
            auto cap2 = quadFan<I>(vertices.size()-circle.size(), vertices.size(), vertices.size(), true);
            vertices.push_back(helix.back());
		    
            indices.insert(indices.end(), cap1.begin(), cap1.end());
		    indices.insert(indices.end(), cap2.begin(), cap2.end());
        }
        setGeometry(QUADS);
	}

	void terrain(){
		terrainFile = ++terrainFile % 5;
		string file = string("terrain")+itos(terrainFile) + ".hm";
		Array2d<float> heights;
		msg("loading terrain map",file);
		heights.load(file.c_str());
		clear();
		Array2d<vec3> vertices(heights.cols,heights.rows);;
		for (U x=0; x < heights.cols; x++)
			for (U z=0; z < heights.rows; z++)
				vertices[x][z] = vec3((x/(float)heights.cols)-.5,heights[x][z]/5.f,-(z/(float)heights.rows)+.5);
		//quads
		#define v(x,y) vertices.index(x,y)
		for (U y=0; y < vertices.rows-1; y++){
			for (U x=0; x < vertices.cols-1; x++){
				indices.push_back(v(x,y));
				indices.push_back(v(x+1,y));
				indices.push_back(v(x+1,y+1));
				indices.push_back(v(x,y+1));
			}
		}

		//trick vector into copying an Array2d
		this->vertices.assign(vertices.value_ptr(), vertices.value_ptr() + vertices.size());
		setGeometry(TERRAIN);
	}

	void frame(){
		FPInput::frame();
		Scene::frame();

		if (keys['>'])
			setWorldTransform(scale(mat4(1),vec3(1) + vec3(Clock::delta)) * *worldTransform);
		if (keys['<'])
			setWorldTransform(scale(mat4(1),vec3(1) - vec3(Clock::delta)) * *worldTransform);
		if (keys['c'])
			Scene::globals.lights[0].color = rainbowColors(.01);

		if (keys['X'])
			setWorldTransform(*worldTransform * scale(mat4(1),vec3(1+Clock::delta,1,1)));
		if (keys['x'])
			setWorldTransform(*worldTransform * scale(mat4(1),vec3(1-Clock::delta,1,1)));
		if (keys['Y'])
			setWorldTransform(*worldTransform * scale(mat4(1),vec3(1,1+Clock::delta,1)));
		if (keys['y'])
			setWorldTransform(*worldTransform * scale(mat4(1),vec3(1,1-Clock::delta,1)));
		if (keys['Z'])
			setWorldTransform(*worldTransform * scale(mat4(1),vec3(1,1,1+Clock::delta)));
		if (keys['z'])
			setWorldTransform(*worldTransform * scale(mat4(1),vec3(1,1,1-Clock::delta)));

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
		if (mouseDown[1]){
			mat4 m = rotate(mat4(1), mouseDelta.x*25.f*Clock::delta, vec3(0,1,0))
				* rotate(mat4(1), -mouseDelta.y*25.f*Clock::delta, right());
			setWorldTransform(m * *worldTransform);
		}
	}

	virtual void passiveMouseMove(int x, int y){
		//inputHandler.passive
	}

	virtual void keyDown (unsigned char key, int x, int y) {
		FPInput::keyDown(key, x, y);
		
		switch (key){
			case 'p':
				setWorldTransform(mat4(1));
				break;
			case 'r':
				toggleReflections();
				break;
			case ',':
				setBlur(*blur-1);
				break;
			case '.':
				setBlur(*blur+1);
				break;
			case '/':
				cout << "vertices / indices: " << vertices.size() << " / " << indices.size() << endl;
				break;
			//experiment with custom shading in surface.frag
			case '1':
			case '2':
			case '3':
				/*
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			*/
				pattern = key-'1';
				break;
			case ' ':
				wireframe = !wireframe;
				break;
		}
	}

	virtual void keyUp(unsigned char key, int x, int y){
		FPInput::keyUp(key, x, y);
	}


	void load(const char* file){
		cout << "loading: " << file << endl;
		ifstream fin(file);
		if (fin.fail()){
			cout << "file couldn't be opened" << endl;
			return;
		}

		clear();

		stringstream ss;
		string line;
		int numVertices, numFaces;
		getline(fin, line);
		//cout << line << endl;
		getline(fin, line);
		//cout << line << endl;
		
		ss << line;
		ss >> numVertices;
		ss >> numFaces;
		cout << "vertices="<<numVertices << " faces="<< numFaces << endl;

		for (int i=0; i < numVertices; i++){
			ss.str("");
			ss.clear();
			getline(fin, line);
			ss << line;
			vec3 v;
			ss >> v.x;
			ss >> v.y;
			ss >> v.z;

			vertices.push_back(v);
		}

		int faceSize=-1;
		for (int i=0; i < numFaces; i++){
			ss.str("");
			ss.clear();
			getline(fin, line);
			ss << line;

			/*use this to allow faces other than triangle, will fail for files with multiple face sizes
			int tmp;
			ss >> tmp;
			assert(faceSize == -1 || faceSize == tmp);
			faceSize = tmp;

			for (int j=0; j < faceSize; j++){
				I index;
				ss >> index;
				indices.push_back(index);
			}
			*/

			ss >> faceSize;
			vector<I> face;
			for (int j=0; j < faceSize; j++){
				I index;
				ss >> index;
				face.push_back(index);
			}

			//convert face into triangles
			for (U j=1; j < face.size()-1; ++j){
				indices.push_back(face[0]);
				indices.push_back(face[j]);
				indices.push_back(face[j+1]);
			}
		}

		setGeometry(TRIANGLES);//getMode(faceSize)
	}

	void print(const char* file){
		cout << "saving geometry to: " << file << endl;
		ofstream out(file);
		out << vertices.size() << " " << indices.size() / 4 << endl;
		for (U i=0; i < vertices.size(); i++)
			out << vertices[i].x << " " << vertices[i].y << " " << vertices[i].z << endl;

		for (U i=0; i < indices.size(); i+=4){
			for (int j=0; j < 4; j++)
				out << indices[i+j] << " ";
			out << endl;
		}
	}

};