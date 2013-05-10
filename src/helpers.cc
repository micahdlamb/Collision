#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include "includes.h"

using namespace std;

//no substr for char*? ... convert to string
string getFileNameWithoutExtension(string path){
	uint b = path.find_last_of( '/' )
		,e = path.find_last_of('.');
	b = b == string::npos ? 0 : (b+1);
	path = path.substr(b, (e==string::npos?path.size():e)-b);
	return path;
}

string getFileExtension(string path){
	return path.substr(path.find_last_of(".") + 1);
}

vec3 rainbowColors(float frequency){
	static int counter = 0;
	counter++;
	float red   = sin(frequency*counter + 0) * 127 + 128;
	float green = sin(frequency*counter + 2) * 127 + 128;
	float blue  = sin(frequency*counter + 4) * 127 + 128;
	return vec3(red/255,green/255,blue/255);
}

bool solveQuadratic(float &s1, float &s2, float a, float b, float c){
	float e = b*b - (a*c)*4;
	if (e < 0)
		return false;//imaginary solution
		
	float discriminant = sqrt(e);
	s1 = (-b - discriminant)/(2*a);
	s2 = (-b + discriminant)/(2*a);
	return true;
}

vec2 ndCoord(vec2 pt,vec2 dim){
	vec2 d = pt / dim;
	d.y = 1 - d.y;
	return d*2.0f - vec2(1);
}


void printDevILErrors(){
	ILenum err;
	while ((err = ilGetError()) != IL_NO_ERROR)
		msg("devIL error", iluErrorString(err));
}

string loadFile(string fname){
	ifstream fin(fname);
	if (!fin.is_open())
		error ("unable to open file: " + fname);
	stringstream ss;
	ss << fin.rdbuf();
	return ss.str();
}

void printGLErrors(const char* e){
	GLenum error;
	while ((error = glGetError()) != GL_NO_ERROR){
		switch (error){
		case GL_INVALID_ENUM: msg(e,"GL_INVALID_ENUM"); break;
		case GL_INVALID_VALUE: msg(e,"GL_INVALID_VALUE"); break;
		case GL_INVALID_OPERATION: msg(e,"GL_INVALID_OPERATION"); break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: msg(e,"GL_INVALID_FRAMEBUFFER_OPERATION"); break;
		case GL_OUT_OF_MEMORY: msg(e,"GL_OUT_OF_MEMORY"); break;
		default: msg(e,"???");
		}
		Sleep(500);
	}
}


int ceilPow2(int val){
	int r = 2;
	for( ; r < val; r*=2 ) {}

	return r;
}

int factorial(int x) {
  return (x == 1 ? x : x * factorial(x - 1));
}

int n_choose_k(int n, int k){
	return factorial(n) / (factorial(k)*factorial(n-k));
}

/*
// loadFile - loads text file into char* 
// allocates memory - so need to delete after use
// size of file returned in fSize
char* loadFile(const char *fname, GLint &fSize){
	ifstream::pos_type size;
	char * memblock;

	// file read based on example in cplusplus.com tutorial
	ifstream file (fname, ios::in|ios::binary|ios::ate);
	if (file.is_open())
	{
		size = file.tellg();
		fSize = (GLuint) size;
		memblock = new char [size];
		file.seekg (0, ios::beg);
		file.read (memblock, size);
		file.close();
		cout << "file " << fname << " loaded" << endl;
	}
	else
		error(string("Unable to open file ")+ fname);
	return memblock;
}
*/

/*
void readOBJ(const char* file, vector<float>& verticesX, vector<float>& textCoordsX)
{
	vector<float> vertices, textCoords;

    FILE *fin;
    fin = fopen(file,"r");
    char line[100];
    float x,y,z;
    unsigned int v1,v2,v3,t1,t2,t3; 
    while(fgets(line,100,fin)!=NULL)
    {
    	 if(line[0] == 'v' && line[1] == ' ')
    	 {
    		  sscanf(line, "v %f %f %f",&x,&y,&z);
    		  vertices.push_back(x);
    		  vertices.push_back(y);
    		  vertices.push_back(z);
    		  continue;
    	 }
    	 if(line[0] == 'v' && line[1] == 't')
    	 {
    		  sscanf(line, "vt %f %f",&x,&y);
    		  textCoords.push_back(x);
    		  textCoords.push_back(y);
    		  continue;
    	 }
    	 if(line[0] == 'f')
    	 {
    		 sscanf(line, "f %d/%d %d/%d %d/%d",&v1,&t1,&v2,&t2,&v3,&t3);
    		 verticesX.push_back(vertices[(v1-1)*3+0]);
    		 verticesX.push_back(vertices[(v1-1)*3+1]);
    		 verticesX.push_back(vertices[(v1-1)*3+2]);
    		 verticesX.push_back(vertices[(v2-1)*3+0]);
    		 verticesX.push_back(vertices[(v2-1)*3+1]);
    		 verticesX.push_back(vertices[(v2-1)*3+2]);
    		 verticesX.push_back(vertices[(v3-1)*3+0]);
    		 verticesX.push_back(vertices[(v3-1)*3+1]);
    		 verticesX.push_back(vertices[(v3-1)*3+2]);
    		 textCoordsX.push_back(textCoords[(t1-1)*2+0]);
    		 textCoordsX.push_back(textCoords[(t1-1)*2+1]);
    		 textCoordsX.push_back(textCoords[(t2-1)*2+0]);
    		 textCoordsX.push_back(textCoords[(t2-1)*2+1]);
    		 textCoordsX.push_back(textCoords[(t3-1)*2+0]);
    		 textCoordsX.push_back(textCoords[(t3-1)*2+1]);
    		 continue;
    	 }				
    }
    fclose(fin);
}
*/

/*
void load_obj(const char* filename, vector<glm::vec3> &vertices, vector<glm::vec3> &normals, vector<GLushort> &elements) {
  ifstream in(filename, ios::in);
  if (!in)
	  error(string("Cannot open ") + filename);
 
  string line;
  while (getline(in, line)) {
	if (line.length() == 0) continue;
	if (line.substr(0,2) == "v ") {
      istringstream s(line.substr(2));
      glm::vec3 v; s >> v.x; s >> v.y; s >> v.z;
      vertices.push_back(v);
    }  else if (line.substr(0,2) == "f ") {
      istringstream s(line.substr(2));
      GLushort a,b,c;
      s >> a; s >> b; s >> c;
      a--; b--; c--;
      elements.push_back(a); elements.push_back(b); elements.push_back(c);
    }
    else if (line[0] == '#') {}
    else {  }
  }
 
  normals.resize(vertices.size(), glm::vec3(0.0, 0.0, 0.0));
  for (int i = 0; i < elements.size(); i+=3) {
    GLushort ia = elements[i];
    GLushort ib = elements[i+1];
    GLushort ic = elements[i+2];
    glm::vec3 normal = glm::normalize(glm::cross(
      vertices[ib] - vertices[ia],
      vertices[ic] - vertices[ia]));
    normals[ia] = normals[ib] = normals[ic] = normal;
  }
}
*/

/**
 * Compile the shader from file 'filename', with error handling

GLint create_shader(const char* filename, GLenum type)
{
  const GLchar* source = read_file(filename);
  if (source == NULL)
    error(string("Error opening shader: ") + filename);

  GLuint res = glCreateShader(type);
  glShaderSource(res, 1, &source, NULL);
  free((void*)source);
 
  glCompileShader(res);
  GLint compile_ok = GL_FALSE;
  glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);
  if (compile_ok == GL_FALSE) {
    printf("%s:", filename);
    print_log(res);
    glDeleteShader(res);
    return 0;
  }
 
  return res;
}
*/

/**
 * Store all the file's contents in memory, useful to pass shaders
 * source code to OpenGL
 
char* read_file(const char* filename)
{
  FILE* in = fopen(filename, "rb");
  if (in == NULL) return NULL;
 
  int res_size = BUFSIZ;
  char* res = (char*)malloc(res_size);
  int nb_read_total = 0;
 
  while (!feof(in) && !ferror(in)) {
    if (nb_read_total + BUFSIZ > res_size) {
      if (res_size > 10*1024*1024) break;
      res_size = res_size * 2;
      res = (char*)realloc(res, res_size);
    }
    char* p_res = res + nb_read_total;
    nb_read_total += fread(p_res, 1, BUFSIZ, in);
  }
 
  fclose(in);
  res = (char*)realloc(res, nb_read_total + 1);
  res[nb_read_total] = '\0';
  return res;
}
*/

// printShaderErrors
// From OpenGL Shading Language 3rd Edition, p215-216
// Display (hopefully) useful error messages if shader fails to compile
/*
void printShaderErrors(GLint shader)
{
	int infoLogLen = 0;
	int charsWritten = 0;
	GLchar *infoLog;

	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

	// should additionally check for OpenGL errors here

	if (infoLogLen > 0)
	{
		infoLog = new GLchar[infoLogLen];
		// error check for fail to allocate memory omitted
		glGetShaderInfoLog(shader,infoLogLen, &charsWritten, infoLog);
		cout << "InfoLog:" << endl << infoLog << endl;
		delete [] infoLog;
	}

	// should additionally check for OpenGL errors here
}
*/

/*
//Bind an attribute, returns location
GLint bind_attribute(GLuint program, const char* attribute_name){
	GLint location = glGetAttribLocation(program, attribute_name);
	if (location == -1)
		printf("Could not bind attribute %s\n", attribute_name);
	return location;
}

//Bind an attribute, returns location
GLint bind_uniform(GLuint program, const char* uniform_name){
	GLint location = glGetUniformLocation(program, uniform_name);
	if (location == -1)
		printf("Could not bind uniform %s\n", uniform_name);
	return location;
}
*/