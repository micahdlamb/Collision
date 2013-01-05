#pragma once

struct Chase : public Object {
	
	
	struct Particle {
		
		enum Type {
			EMITTER
			,NORMAL
		};
		
		float type;
		vec3 color;
		vec3 pos;
		vec3 vel;
		float age;
	};

	static const int MAX_PARTICLES = 10000;

	Uniform3f chasePt;
	Uniform1f acc;
	Uniform1f maxSpeed;

	UniformSampler randDirs;
	FeedbackLoop looper;

	void operator()(mat4 transform){
		Object::operator()(transform,"chase.vert", "chase.frag", "chase.geom");

		Particle* particles = new Particle[MAX_PARTICLES];
		particles[0].type = Particle::EMITTER;
		particles[0].color = vec3(0);
		particles[0].pos = vec3(0);
		particles[0].vel = vec3(0);
		particles[0].age = 0.0f;

		for (int i=0; i < 5; i++){
			particles[i].type = Particle::NORMAL;
			particles[i].color = vec3(rand0_1,rand0_1,rand0_1);
			particles[i].pos = vec3(rand0_1-.5,rand0_1-.5,rand0_1-.5)*50.f;
			particles[i].vel = vec3(0);
			particles[i].age = 0.0f;
		}

		looper(5);
		looper.buffer(particles,sizeof(Particle)*MAX_PARTICLES);
		delete particles;
		looper.inout("oType",1,GL_FLOAT,sizeof(Particle),0);
		looper.inout("oColor",3,GL_FLOAT,sizeof(Particle),4);
		looper.inout("oPos",3,GL_FLOAT,sizeof(Particle),16);
		looper.inout("oVel",3,GL_FLOAT,sizeof(Particle),28);
		looper.inout("oAge",1,GL_FLOAT,sizeof(Particle),40);

		looper.finalize("chase_update.vert","chase_update.geom");
		looper.enable();
		chasePt("chasePt",&looper);
		acc("acc",&looper,1);
		maxSpeed("maxSpeed",&looper,3);
		randDirs("randDirs",&looper,randomDirections(2048));
	}

	Texture* randomDirections(int size){
		vec3* dirs = new vec3[size];
		for (int i=0; i < size; i++)
			dirs[i] = vec3(rand0_1,rand0_1,rand0_1);

		Texture1D* t = new Texture1D;
		t->operator()(dirs,size,GL_RGB,GL_RGB,GL_FLOAT);
		delete dirs;
		return t;
	}

	void setChasePt(vec3 pt){
		looper.enable();
		chasePt = pt;
	}

	void draw(){
		looper.enable();
		
		looper.loop();
		Shader::enable();
		//glDepthMask(GL_FALSE);
		Object::ready();
		looper.draw();
		//glDepthMask(GL_TRUE);
	}
};