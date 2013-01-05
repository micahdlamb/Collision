#pragma once

struct MyWindow : public ViewportManager {
	
	void operator()(){
		ViewportManager::operator()();
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	void draw(){
		ViewportManager::draw();
		glutSwapBuffers();
	}
};