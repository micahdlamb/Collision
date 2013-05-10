struct Kinematic {
	vec3 pos, vel, momentum;
	float mass, rotationalInertia;
	vec3 force, torque, angularMomentum, angularVel;
	quat orientation, spin;

	Kinematic(vec3 pos, vec3 vel, float mass):pos(pos),vel(vel),mass(mass){}
	
	void recalc(){
		//vel = momentum / mass;
		orientation = normalize(orientation);
		spin = .5 * quat(0, angularVel) * orientation;
	}
	
	void setVel(vec3 v){
		vel = v;
		momentum = mass * vel;
	}

	void setPos(vec3 p){
		pos = p;
	}

	//need to use real integrater eventually
	void integrate(float dt){
		vec3 vel0 = vel;
		vel += force * dt / mass;
		pos += (vel0 + vel) * .5f * dt;
		//momentum += force * dt;

		orientation = orientation + spin * dt;
		angularMomentum += torque * dt;
		angularVel = angularMomentum / rotationalInertia;
		orientation = normalize(orientation);
		spin = .5 * quat(0, angularVel) * orientation;
		//recalc();
	}

	void clear(){
		force = vec3(0);
		torque = vec3(0);
	}

	vec3 ptVel(vec3 pt){
		return vel + cross(angularVel, pt - pos);
	}

	void makeSphere(float radius){
		rotationalInertia = 2 * mass * radius * radius / 5.f;
	}
};

struct Ball : public Kinematic, public Pickable {
	Uniform3f color;
	Uniform1i asleep;
	UniformSampler shadowMap;
	float size;

	float still;
	int sleepTicks;
	int stillTicks;

	Ball(VAO* vao, Texture* shadowMap, int id, vec3 pos, float size, vec3 color, SHADER_PARAMS):
		Pickable(translate(mat4(1),pos)*scale(mat4(1),vec3(size)), id, SHADER_CALL)
		,Kinematic(pos, vec3(0), 0)
		,still(.5)
		,stillTicks(0)
		,sleepTicks(100)
		,size(size)
	{
		setVAO(vao);
		asleep("asleep",this,0);
		this->shadowMap("shadowMap",this,shadowMap);
		float radius = dynamic_cast<BoundingSphere*>(boundingVolume)->radius;
		mass = radius;
		makeSphere(radius);
		this->color("color",this,color);
		enableShadowCast();
	}

	void go(float dt, vec3 gravity=vec3(0)){
		
		if (!*selected){
			force += gravity / mass;
			integrate(dt);
			updateBV();
		} else
			angularMomentum = vec3(0);
		clear();
	}
	void move(float dt, vec3 gravity=vec3(0)){
		int accuracy = 25;
		dt /= accuracy;
		for (int i=0; i < accuracy; i++){
			go(dt,gravity*.2f);
		}
	}

	void dampen(){

		
		const float linear = 0.1f;
		const float angular = 0.1f;
		
		force -= linear * vel;
		torque -= angular * angularVel;

		//vel *= .9f;
		//angularVel *= .9f;
	}

	mat4 getWorldTransform(){
		return translate(mat4(1), pos) * mat4_cast(orientation) * scale(mat4(1),vec3(size));
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

		//if (!sleeping())
			setWorldTransform(getWorldTransform());
		setAsleep(sleeping()?1:0);
	}

	void setPos(vec3& val){
		pos = val;
		updateBV();
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
		collide();
		for (auto i=objects.begin(); i != objects.end(); i++){
			(*i)->dampen();
			(*i)->move(step, acc);
		}
	}

	//auto bounds = terrainDim * .5f - 1.f;
	void collideWithTerrain(ITerrain* terrain, vec2 bounds, float damp=.5){
		#pragma omp parallel for
		for (int i=0; i < objects.size(); i++){
			auto b = objects[i];
			if (b->sleeping()) continue;

			//keep in terrain
			auto& pos = b->pos;
			auto s = dynamic_cast<BoundingSphere*>(b->boundingVolume);
			
			if (pos.x > bounds.x){
				collide(*b, vec3(-1,0,0), b->ptVel(pos+vec3(s->radius,0,0)), pos.x - bounds.x);
			}
			if (pos.x < -bounds.x){
				collide(*b, vec3(1,0,0), b->ptVel(pos+vec3(-s->radius,0,0)), -bounds.x - pos.x);
			}
			if (pos.z > bounds.y){
				collide(*b, vec3(0,0,-1), b->ptVel(pos+vec3(0,0,s->radius)), pos.z - bounds.y);
			}
			if (pos.z < -bounds.y){
				collide(*b, vec3(0,0,1), b->ptVel(pos+vec3(0,0,-s->radius)), -bounds.y - pos.z);
			}

			float y = terrain->getY(s->center.x,s->center.z) + s->radius;
			if (s->center.y < y){
				auto normal = terrain->getNormal(s->center.x,s->center.z);
				collide(*b, normal, b->ptVel(pos+vec3(0,-s->radius,0)), y - s->center.y);
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
				collide(*b, vec3(-1,0,0), b->ptVel(pos+vec3(s->radius,0,0)), pos.x - bounds.x);
			}
			if (pos.x < -bounds.x){
				collide(*b, vec3(1,0,0), b->ptVel(pos+vec3(-s->radius,0,0)), -bounds.x - pos.x);
			}
			if (pos.z > bounds.z){
				collide(*b, vec3(0,0,-1), b->ptVel(pos+vec3(0,0,s->radius)), pos.z - bounds.z);
			}
			if (pos.z < -bounds.z){
				collide(*b, vec3(0,0,1), b->ptVel(pos+vec3(0,0,-s->radius)), -bounds.z - pos.z);
			}
			if (pos.y > bounds.y){
				collide(*b, vec3(0,-1,0), b->ptVel(pos+vec3(0,s->radius,0)), pos.y - bounds.y);
			}
			if (pos.y < s->radius){
				collide(*b, vec3(0,1,0), b->ptVel(pos+vec3(0,-s->radius,0)), s->radius - pos.y);
			}
		}
	}

	void collideWithViewer(vec3 pos, vec3 vel, float radius, float mass=1000000){
		BoundingSphere eyeBound(pos, radius);
		Kinematic eyeBall(pos,vel,mass);

		for (auto i=objects.begin(); i != objects.end(); i++){
			auto bs = (BoundingSphere*)(*i)->boundingVolume;
			//collide with eye
			auto r = eyeBound.intersect(bs);
			if (r.intersect){
				collide(**i, normalize(eyeBall.pos - (*i)->pos), (*i)->vel - vel, eyeBound.radius + bs->radius - length(eyeBall.pos - bs->center));
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
		handleCollision(b1,b2,r.pt);
	}

	void collide(Ball& ball, vec3 normal, vec3 vel, float penetration){
		penetration *= 10;
		const float c = 10*10;
		const float k = 100*10;
		const float b = 5*10;
		const float f = 3*10;

		float rel = -dot(normal, vel);

		//collision force
		ball.force += normal * rel * c;

		vec3 tangentialVel = vel + (normal * rel);
		//friction force
		vec3 frictionForce = -tangentialVel * f;
		ball.force += frictionForce;
		ball.torque += cross(-normal, frictionForce);

		vec3 penaltyForce = normal * penetration * k;

		//penalty force
		ball.force += penaltyForce;

		ball.torque += cross(-normal, penaltyForce);

		//damping force
		vec3 dampingForce1 = normal * rel * penetration * b;
		ball.force += dampingForce1;
		ball.torque += cross(-normal, dampingForce1);
	}

	//separate objects and make sure they are moving towards each other, then collide them
	void handleCollision(Ball& b1, Ball& b2, vec3 pt){
		auto s1 = dynamic_cast<BoundingSphere*>(b1.boundingVolume);
		auto s2 = dynamic_cast<BoundingSphere*>(b2.boundingVolume);

		auto v = s2->center - s1->center;
		float dist = length(v);
		float overlap = s1->radius + s2->radius - dist;
		vec3 norm1 = s1->center - s2->center;
		vec3 norm2 = -norm1;
		
		collide(b1, normalize(s1->center - s2->center), b1.ptVel(pt) - b2.ptVel(pt), overlap);
		collide(b2, normalize(s2->center - s1->center), b2.ptVel(pt) - b1.ptVel(pt), overlap);

		/*
		float rel1 = -dot(norm1, b1.vel);
		float rel2 = -dot(norm2, b2.vel);

		//collision force
		b1.force += norm1 * rel1 * c;
		b2.force += norm2 * rel2 * c;

		vec3 tangentialVel = b1.vel + (norm1 * rel1);
		//friction force
		vec3 frictionForce1 = -(b1.vel + (norm1 * rel1)) * f;
		vec3 frictionForce2 = -(b2.vel + (norm2 * rel2)) * f;
		b1.force += frictionForce1;
		b2.force += frictionForce2;

		b1.torque += cross(-norm1, frictionForce1);
		b2.torque += cross(-norm2, frictionForce2);

		vec3 penaltyForce1 = norm1 * overlap * k;
		vec3 penaltyForce2 = norm2 * overlap * k;

		//penalty force
		b1.force += penaltyForce1;
		b2.force += penaltyForce2;

		b1.torque += cross(-norm1, penaltyForce1);
		b2.torque += cross(-norm2, penaltyForce2);

		//damping force
		vec3 dampingForce1 = norm1 * rel1 * overlap * b;
		vec3 dampingForce2 = norm2 * rel2 * overlap * b;

		b1.force += dampingForce1;
		b2.force += dampingForce2;

		b1.torque += cross(-norm1, dampingForce1);
		b2.torque += cross(-norm2, dampingForce2);
		*/
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