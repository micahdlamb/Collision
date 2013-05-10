#pragma once

//inherit from this to make the object selectable with mouse
struct Pickable : Object {
	Shader shader;
	Uniform1i id;
	Uniform1i selected;

	Pickable(mat4 transform, int id,SHADER_PARAMS):Object(transform,SHADER_CALL){
		shader("pick.vert","pick.frag");
		this->id("id",&shader,id);
		Shader::enable();
		selected("selected",this,0);
		Object::addShader(&shader);
	}

	void pickDraw(){
		if (cull()) return;
		shader.enable();
		Object::ready();
		vao->draw();
	}

	void setSelected(int v){
		Shader::enable();
		selected = v;
	}
};