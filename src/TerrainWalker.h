#pragma once

struct TerrainWalker : public InputHandler, public Viewer {
	float moveSpeed;
	float acc;
	float sensitivity;

	ITerrain* terrain;

	float viewerHeight;
	bool jumping;
	vec3 vel, gravity;
	float jumpVel;
	float bounce;

	void operator()(ITerrain* terrain, float moveSpeed=1, float sensitivity=.1, float viewerHeight=2, float jumpVel=10, float gravity=9.8, float acc=8, float bounce=.1){
		this->terrain = terrain;
		this->moveSpeed = moveSpeed;
		this->acc = acc;
		this->sensitivity = sensitivity;
		this->viewerHeight = viewerHeight;
		this->jumpVel = jumpVel;
		this->gravity = vec3(0,-gravity,0);
		this->bounce = bounce;
		this->jumping = false;
	}

	void moveForward(float v){
		pos += forward * v;
	}

	void strafeRight(float v){
		pos += right() * v;
	}
	void moveUp(float v){
		pos += up() * v;
	}

	virtual void mouseButton(int button, int state, int x, int y){
		InputHandler::mouseButton(button,state,x,y);
	}

	virtual void mouseMove(int x, int y){
		InputHandler::mouseMove(x,y);

		if (mouseDown[0])
			turn(mouseDelta.x*sensitivity,mouseDelta.y*sensitivity);
	}

	virtual void keyDown(unsigned char key, int x, int y){
		InputHandler::keyDown(key,x,y);
		switch (key){
			case '=' : moveSpeed+=1+sqrt(moveSpeed); break;
			case '-' : moveSpeed/=2; break;
		}
	}

	virtual void keyUp(unsigned char key, int x, int y){
		InputHandler::keyUp(key,x,y);
	}

	vec3 getInput(){
		vec3 r(0);
		for (int i=0; i < 256; i++){
			if (keys[i])
				switch (i) {
					case 'w' : r += forward; break;
					case 's' : r -= forward;  break;
					case 'a': r -= right(); break;
					case 'd': r += right(); break;
					case 'e': r += up(); break;
					case 'q': r -= up(); break;
				}
		}
		return r;
	}

	virtual void frame(){
		auto input = getInput();
		float y = terrain->getY(pos.x,pos.z);
		if (y == -1){
			vel = vec3(0);
			pos += input * moveSpeed * Clock::delta;
			return;
		}
		y += viewerHeight;
		input.y = 0;
		
		float inputMag = length(input);
		if (inputMag)
			input = normalize(input);

		bool airborn = 	pos.y > y;
		if (!airborn){
			jumping = false;
			vel.y = std::max(abs(vel.y)*bounce, vel.y);
			if (inputMag){
				auto normal = terrain->getNormal(pos.x,pos.z);
				auto right = cross(input,world_up);
				auto uphill = cross(normal,right);
				if (uphill.y > 0){
					uphill = normalize(uphill);
					//if (dot(uphill,vec3(0,1,0)) > .95)
						//return;
					input = normalize(uphill);
				}
			}
		} else
			vel += gravity * Clock::delta;
		
		if (inputMag){
			//accelerate
			vel += input * moveSpeed * acc * Clock::delta;
			auto horiz = vec2(vel.x,vel.z);//vel.xz();
			float speed = airborn ? length(horiz) : length(vel);

			if (speed > moveSpeed){
				if (airborn){
					horiz *= moveSpeed / speed;
					vel = vec3(horiz.x,vel.y,horiz.y);
				} else
					vel *= moveSpeed / speed;
			}
		} else {
			//deaccelerate
			auto horiz = vec2(vel.x,vel.z);
			float speed = length(horiz);
			if (speed){
				auto drag = Clock::delta * horiz * acc * moveSpeed / speed;
				auto prev = horiz;
				horiz -= drag;
				if (prev.x > 0 != horiz.x > 0) horiz.x = 0;
				if (prev.y > 0 != horiz.y > 0) horiz.y = 0;
				vel = vec3(horiz.x,vel.y,horiz.y);
			}
			
			if (airborn && !jumping && vel.y > 0)
				vel.y -= Clock::delta * vel.y * acc * moveSpeed / vel.y;

			/*
			if (airborn){
				auto horiz = vec2(vel.x,vel.z);//vel.xz();
				float speed = length(horiz);
				if (speed){
					auto drag = Clock::delta * horiz * acc * moveSpeed / speed;
					auto prev = horiz;
					horiz -= drag;
					if (prev.x > 0 != horiz.x > 0) horiz.x = 0;
					if (prev.y > 0 != horiz.y > 0) horiz.y = 0;
					vel = vec3(horiz.x,vel.y,horiz.y);
				}
			} else {
				float speed = length(vel);
				if (speed){
					auto drag = Clock::delta * vel * acc * moveSpeed / speed;
					auto prev = vel;
					vel -= drag;
					if (prev.x > 0 != vel.x > 0) vel.x = 0;
					if (prev.y > 0 != vel.y > 0) vel.y = 0;
					if (prev.y > 0 != vel.z > 0) vel.z = 0;
				}
			}
			*/
		}

		pos += vel * Clock::delta;
		y = terrain->getY(pos.x,pos.z) + viewerHeight;
		/*
		if (pos.y <= y)
			vel.y = 0;
		*/
		pos.y = std::max(pos.y, y);
	}

	void jump(){
		pos.y += FLT_EPSILON;
		vel.y = jumpVel;
		jumping = true;
	}
};
