#pragma once

struct MyFireworks : public Object {
	
	//#define MAX_PARTICLES 1000000
	static const int MAX_PARTICLES = 1000000;
	#define PARTICLE_LIFETIME 10.0f
	//static const float PARTICLE_LIFETIME = 10.f;
	#define PARTICLE_TYPE_LAUNCHER 0.0f
	//static const float PARTICLE_TYPE_LAUNCHER = 0.0f;
	#define PARTICLE_TYPE_SHELL 1.0f
	//static const float PARTICLE_TYPE_SHELL = 1.0f;
	#define PARTICLE_TYPE_SECONDARY_SHELL 2.0f
	//static const float PARTICLE_TYPE_SECONDARY_SHELL = 2.0f;
	
	struct Particle
	{
		float Type;    
		vec3 Pos;
		vec3 Vel;    
		float LifetimeMillis;
		float Color;
	};

	Uniform1f gDeltaTimeMillis;
	Uniform1f gTime;
	UniformSampler gRandomTexture;
	UniformSampler gColorMap;

	FeedbackLoop looper;

	void operator()(mat4 transform){
		Object::operator()(transform,"fireworks.vert", "fireworks.frag", "fireworks.geom");
		gColorMap("gColorMap",this,new ILTexture("fireworks_red.jpg"));

		Particle* particles = new Particle[MAX_PARTICLES];
		particles[0].Type = PARTICLE_TYPE_LAUNCHER;
		particles[0].Pos = vec3(0);
		particles[0].Vel = vec3(0.0f, 0.0001f, 0.0f);
		particles[0].LifetimeMillis = 0.0f;
		particles[0].Color = 0;

		looper(1);
		looper.buffer(particles,sizeof(Particle)*MAX_PARTICLES);
		delete particles;
		looper.inout("Type1",1,GL_FLOAT,sizeof(Particle));
		looper.inout("Position1",3,GL_FLOAT,sizeof(Particle),4);
		looper.inout("Velocity1",3,GL_FLOAT,sizeof(Particle),16);
		looper.inout("Age1",1,GL_FLOAT,sizeof(Particle),28);
		looper.inout("Color1",1,GL_FLOAT,sizeof(Particle),32);
		looper.finalize("fireworks_update.vert","fireworks_update.geom");
		looper.enable();
		gDeltaTimeMillis("gDeltaTimeMillis",&looper);
		gTime("gTime",&looper);
		gRandomTexture("gRandomTexture",&looper,randomDirections(2048));

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

	void draw(){
		looper.enable();
		gDeltaTimeMillis = Clock::deltaMilli;
		gTime = Clock::timeMilli;
		
		looper.loop();
		Shader::enable();
		glDepthMask(GL_FALSE);
		Object::ready();
		looper.draw();
		glDepthMask(GL_TRUE);
	}
};