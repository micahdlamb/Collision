#pragma once

struct FeedbackLoop : public Shader {
	bool first;
	GLuint size;

	GLenum mode, hint;
	GLuint i;

	bool toggle;

	typedef glm::detail::tvec2<GLuint> I2;//because GLuint[2] errors
	I2 vaos;
	I2 tfs;
	vector<I2> bufs;
	vector<char*> outNames;

	FeedbackLoop(GLenum mode = GL_TRIANGLES, GLenum hint = GL_DYNAMIC_DRAW): mode(mode),hint(hint),i(0),toggle(true),first(true){}
	
	void operator()(GLuint size){
		this->size = size;
		glGenVertexArrays(2, (GLuint*)&vaos);
	}
	
	void buffer(void* data, int size){
		printGLErrors("tf buffer");
		if (bufs.size())
			outNames.push_back("gl_NextBuffer");
		
		I2 buffers;
		glGenBuffers(2, (GLuint*)&buffers);
		glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
		glBufferData(GL_ARRAY_BUFFER, size, data, hint);
		glBindBuffer(GL_ARRAY_BUFFER, buffers[1]);
		glBufferData(GL_ARRAY_BUFFER, size, NULL, hint);
		bufs.push_back(buffers);
		printGLErrors("/tf buffer");
	}
	
	void inout(char* outName, int elementSize, GLenum type, GLuint stride=0, GLuint start=0){
		printGLErrors("inout");
		outNames.push_back(outName);
		
		glBindVertexArray(vaos[0]);
		glBindBuffer(GL_ARRAY_BUFFER, bufs.back()[0]);
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, elementSize, type, GL_FALSE, stride, (void*)start);
		glBindVertexArray(vaos[1]);
		glBindBuffer(GL_ARRAY_BUFFER, bufs.back()[1]);
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(i, elementSize, type, GL_FALSE, stride, (void*)start);
		glBindVertexArray(0);
		i++;
		
		printGLErrors("/inout");
	}

	void finalize(const char* vert, const char* geo=NULL){
		printGLErrors("finalize");
		Shader::operator()(vert,NULL,geo,NULL,NULL,false);
		glTransformFeedbackVaryings(Shader::gid, outNames.size(), (const GLchar**)&outNames[0], GL_INTERLEAVED_ATTRIBS);
		Shader::link();
		glGenTransformFeedbacks(2, (GLuint*)&tfs);    

		for (int i=0; i < 2; i++){
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfs[i]);
			for (GLuint j=0; j < bufs.size(); j++){
				glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, j, bufs[j][i]);
			}
		}
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
		printGLErrors("/finalize");
	}

	//must call if buffers updated
	void reset(GLuint size){
		first = true;
		this->size = size;
		toggle = true;
	}

	void loop(GLuint num=0){
		printGLErrors("loop");
		Shader::enable();
		glEnable(GL_RASTERIZER_DISCARD);
		
		glBindVertexArray(vaos[toggle?0:1]);
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, tfs[toggle?1:0]);

		glBeginTransformFeedback(GL_POINTS);
		if (first){
			if (!num) num = size;
			first = false;
			glDrawArrays(GL_POINTS,0,num);
		}
		else
			glDrawTransformFeedback(GL_POINTS, tfs[toggle?0:1]);
		glEndTransformFeedback();
		
		glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
		glBindVertexArray(0);
		
		glDisable(GL_RASTERIZER_DISCARD);
		toggle = !toggle;
		printGLErrors("/loop");
	}

	void draw(GLenum mode=-1){//GL_POINTS stole 0 :(
		printGLErrors("tf draw");
		if (mode==-1)
			mode = this->mode;

		glBindVertexArray(vaos[toggle?0:1]);
		glDrawTransformFeedback(GL_POINTS, tfs[toggle?0:1]);
		glBindVertexArray(0);

		printGLErrors("/tf draw");
	}


};
