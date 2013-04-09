struct Kinematic {
	Kinematic(vec3 pos, vec3 vel, float mass):pos(pos),vel(vel),mass(mass){}
	vec3 pos, vel;
	float mass;
};

struct Ball : public Kinematic, public Pickable {
	Uniform3f color;
	Uniform1i asleep;
	UniformSampler shadowMap;
	
	float still;
	int sleepTicks;
	int stillTicks;

	Ball(VAO* vao, Texture* shadowMap, int id, vec3 pos, float size, vec3 color, SHADER_PARAMS):
		Pickable(translate(mat4(1),pos)*scale(mat4(1),vec3(size)), id, SHADER_CALL)
		,Kinematic(pos, vec3(0), 0)
		,still(.5)
		,stillTicks(0)
		,sleepTicks(100)
	{
		setVAO(vao);
		asleep("asleep",this,0);
		this->shadowMap("shadowMap",this,shadowMap);
		mass = dynamic_cast<BoundingSphere*>(boundingVolume)->radius;
		this->color("color",this,color);
		enableShadowCast();
	}
	void move(float step, vec3 acc=vec3(0)){
		if (trySleep()) return;
		//vel += acc * step;
		//pos += vel * step;
		vec3 vel0 = vel;
		vel += acc * step;
		pos += (vel + vel0) * .5f * step;
		updateBV();
	}

	mat4 getWorldTransform(){
		mat4 m = Object::getWorldTransform();
		m[3] = vec4(pos,1);//only change translation part
		return m;
	}

	//update bounding volume (vao->boundingVolume is the bv in local space)
	void updateBV(){
		vao->boundingVolume->transform(getWorldTransform(), boundingVolume);
	}

	void commit(){
		if (NaN(pos.x) || NaN(vel.x)){
			cout << "wierd ball fuck up" << endl;
			pos = vec3(0,100,0);
			vel = vec3(0);
		}

		if (!sleeping())
			setWorldTransform(getWorldTransform());
		setAsleep(sleeping()?1:0);
	}

	void setAsleep(int v){
		Shader::enable();
		asleep = v;
	}

	/*
	handle the sleep optimization
	once object is moving slower than still for more than sleepTicks frames set it to be asleep
	Once asleep:
		the object is no longer affected by gravity or moves, no need to update bounding volume
		no need to check for intersection between 2 sleeping objects
		no more need to check for intersection with terrain and compute reflections
	*/
	bool isStill(){
		return dot(vel,vel) < still;
	}

	bool trySleep(){
		if (isStill())
			stillTicks++;
		else
			wakeUp();

		Object::boundingVolume->sleeping = sleeping();
		if (sleeping())
			vel = vec3(0);
		return sleeping();
	}

	bool sleeping(){
		return stillTicks > sleepTicks;
	}
	
	void wakeUp(){
		stillTicks = 0;
		Object::boundingVolume->sleeping = false;
	}
};


struct BallManager : public Grid2D::IntersectionHandler {
	vector<Ball*> objects;
	vector<IBoundingVolume*> bvs;
	Grid2D grid;

	void operator()(vec2 worldMin, vec2 worldMax, uvec2 numCells){
		grid(worldMin, worldMax, numCells);
	}

	void add(Ball* ball){
		objects.push_back(ball);
		bvs.push_back(ball->boundingVolume);
	}

	void animate(float step, vec3 acc=vec3(0)){
		for (auto i=objects.begin(); i != objects.end(); i++){
			if (*(*i)->selected) continue;
			(*i)->move(step, acc);
		}

		collide();
	}

	//auto bounds = terrainDim * .5f - 1.f;
	void collideWithTerrain(ITerrain* terrain, vec2 bounds, float damp=.5){
		#pragma omp parallel for
		for (int i=0; i < objects.size(); i++){
			auto b = objects[i];
			if (b->sleeping()) continue;

			//keep in terrain
			auto& pos = b->pos;
			auto& vel = b->vel;
			auto s = dynamic_cast<BoundingSphere*>(b->boundingVolume);
			
			if (pos.x > bounds.x){
				pos.x = bounds.x;
				vel.x *= -damp;
			}
			if (pos.x < -bounds.x){
				pos.x = -bounds.x;
				vel.x *= -damp;
			}
			if (pos.z > bounds.y){
				pos.z = bounds.y;
				vel.z *= -damp;
			}
			if (pos.z < -bounds.y){
				pos.z = -bounds.y;
				vel.z *= -damp;
			}

			float y = terrain->getY(s->center.x,s->center.z) + s->radius;
			if (s->center.y < y && dot(vel,vel) > 0){
				pos.y = y + pos.y - s->center.y;
				auto normal = terrain->getNormal(s->center.x,s->center.z);
				vel = reflect(b->vel,normal) * (1 - dot(normalize(-vel), normal) * (1-damp));
			}
		}
	}

	void collideWithWalls(vec3 bounds, float damp=.5){
		for (auto i=objects.begin(); i != objects.end(); i++){
			auto b = *i;
			if (b->sleeping()) continue;

			//keep in terrain
			auto& pos = b->pos;
			auto& vel = b->vel;
			auto s = dynamic_cast<BoundingSphere*>(b->boundingVolume);
			vec3 walls = bounds - vec3(s->radius);
			if (pos.x > bounds.x){
				pos.x = bounds.x;
				vel.x *= -damp;
			}
			if (pos.x < -bounds.x){
				pos.x = -bounds.x;
				vel.x *= -damp;
			}
			if (pos.z > bounds.z){
				pos.z = bounds.z;
				vel.z *= -damp;
			}
			if (pos.z < -bounds.z){
				pos.z = -bounds.z;
				vel.z *= -damp;
			}
			if (pos.y > bounds.y){
				pos.y = bounds.y;
				vel.y *= -damp;
			}
			if (pos.y < s->radius){
				pos.y = s->radius;
				vel.y *= -damp;
			}
		}
	}

	void collideWithViewer(vec3 pos, vec3 vel, float radius, float mass=1000000){
		BoundingSphere eyeBound(pos, radius);
		Kinematic eyeBall(pos,vel,mass);

		for (auto i=objects.begin(); i != objects.end(); i++){
			//collide with eye
			auto r = eyeBound.intersect((*i)->boundingVolume);
			if (r.intersect && dot((*i)->vel - eyeBall.vel, eyeBall.pos - (*i)->pos) > FLT_EPSILON){
				collide(**i, eyeBall, eyeBall.pos - (*i)->boundingVolume->center);
			}
		}
	}

	//update shaders with new object positions
	void sync(){
		for (auto i=objects.begin(); i != objects.end(); i++)
			(*i)->commit();
	}

	//pass bounding volumes to grid intersection routine
	void collide(){
		grid.intersect(bvs, this);
	}

	//handle intersection by calling handle collision
	void handleIntersection(Grid2D::Result r){
		auto& b1 = *objects[r.first];
		auto& b2 = *objects[r.second];
		handleCollision(b1,b2);
	}

	//separate objects and make sure they are moving towards each other, then collide them
	void handleCollision(Ball& b1, Ball& b2){
		auto s1 = dynamic_cast<BoundingSphere*>(b1.boundingVolume);
		auto s2 = dynamic_cast<BoundingSphere*>(b2.boundingVolume);

		auto v = s2->center - s1->center;
		float dist = length(v);
		if (dist > 0){
			v = normalize(v);

			//separate bvs
			float overlap = s1->radius + s2->radius - dist;
			if (!b1.sleeping())
				b1.pos -= v * overlap * .5f * (dot(b1.vel,b1.vel) < 5 ? .2f : 1.f);
			if (!b2.sleeping())
				b2.pos += v * overlap * .5f * (dot(b2.vel,b2.vel) < 5 ? .2f : 1.f);

			//do nothing if movement very slow or moving away from each other
			if (dot(b1.vel - b2.vel, b2.pos - b1.pos) <= FLT_EPSILON) return;//fix balls getting stuck in each other
			collide(b1, b2, v);
		}
	}

	//conservation of momentum (without angular velocity yet)
	static void collide(Kinematic& b1, Kinematic& b2, vec3 n){
		float e = 1;
		float v1i = dot(b1.vel, n);
		float v2i = dot(b2.vel, n);

		vec3 v = b1.vel - b2.vel;
		float j = (-(1+e)*dot(v,n)) / (dot(n,n)*(1/b1.mass + 1/b2.mass));
		b1.vel += (j / b1.mass)*n;
		b2.vel -= (j / b2.mass)*n;
	}

	void draw(){
		for (auto i=objects.begin(); i!=objects.end(); i++)
			(*i)->draw();
	}

	void pickDraw(){
		for (auto i=objects.begin(); i!=objects.end(); i++)
			if (dynamic_cast<Pickable*>(*i))
				((Pickable*)(*i))->pickDraw();
	}

	void shadowDraw(){
		for (auto i=objects.begin(); i!=objects.end(); i++)
			(*i)->shadowDraw();
	}
};