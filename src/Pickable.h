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

	static Texture* tex;

	void static bind(Viewport* viewport){
		tex = tex ? tex : new Texture(NULL,2048,2048,GL_RGB32UI,GL_RGB_INTEGER,GL_UNSIGNED_INT);
		viewport->enable();
		glClearColor(1,1,1,1);
		tex->bind2FB(false);
	}

	void static unbind(){
		tex->unbind2FB();
		Viewport::pop();//same as disable()
	}

	uint static getId(int x, int y){
		uvec3 id;
		readFB(&id,x,y,1,1,GL_RGB_INTEGER,GL_UNSIGNED_INT);
		return id.x;
	}
};