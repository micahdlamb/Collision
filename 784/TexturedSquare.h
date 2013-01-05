#pragma once;

struct TexturedSquare : public Shader {
	UniformSampler tex;
	void operator()(Texture* texture){
		Shader::operator()("texturedSquare.vert","texturedSquare.frag");
		tex("tex",this,texture);
	}

	void draw(){
		Shader::enable();
		Shapes::square()->draw();
	}

};