#pragma once
struct MyTerrain : public Object, public ITerrain {
	typedef GLuint I;
	#define ITYPE GL_UNSIGNED_INT

	Array2d<float> heights;
	Array2d<vec3> normals;

	UniformSampler noise;
	UniformSampler water;
	UniformSampler grass;
	UniformSampler stone;
	UniformSampler rock;
	UniformSampler snow;
	UniformSampler pebbles;

	UniformSampler skyBox;

	UniformSampler heightMap;
	UniformSampler normalMap;

	UniformSampler shadowMap;

	Uniform1f lod;

	//shadow shader, since cant share samplers between shaders for some reason
	UniformSampler shadowHeightMap;
	Uniform1f shadowLod;

	Uniform1f waveAmp;

	MyTerrain(const char* file, mat4 transform, Texture* background, int patchSize, Texture* shadowMap):
		Object(transform * scale(mat4(1),vec3(1,1,-1)) * translate(mat4(1), vec3(-.5,-.02,-.5)), "tess.vert","tess.frag","tess.geom","tess.tcs.glsl","tess.tes.glsl"){
		noise("noise",this,new ILTexture("MyTerrain/noise.png"));
		water("water",this,new ILTexture("MyTerrain/water.jpg"));
		grass("grass",this,new ILTexture("MyTerrain/grass.jpg"));
		stone("stone",this,new ILTexture("MyTerrain/stone.jpg"));
		rock("rock",this,new ILTexture("MyTerrain/rock.jpg"));
		snow("snow",this,new ILTexture("MyTerrain/snow.jpg"));
		pebbles("pebbles",this,new ILTexture("MyTerrain/pebbles.jpg"));
		skyBox("skyBox",this,background);

		lod("lod",this,.1f);
		waveAmp("waveAmp",this);

		this->shadowMap("shadowMap",this,shadowMap);

		msg("loading terrain map",file);
		heights(file);
		heightMap("heightMap",this,new Texture(heights.value_ptr(),heights.cols,heights.rows,GL_R32F,GL_RED,GL_FLOAT,false,GL_CLAMP_TO_EDGE));
	
		int patchesX = heights.cols / patchSize
			,patchesY = heights.rows / patchSize;

		Array2d<vec2> vertices(patchesX,patchesY);
		for (size_t x=0; x < vertices.cols; x++)
			for (size_t y=0; y < vertices.rows; y++)
				vertices[x][y] = vec2((float)x/(vertices.cols-1), (float)y/(vertices.rows-1));

		auto indices = gridIndices<I>(vertices.size(),vertices.rows,false,false);

		//calc normal texture
		string normalsFile = string(file)+".normals";
		if (normals.load(normalsFile.c_str()))
			msg("normals loaded from", normalsFile);
		else {
			msg("calculating normals","");
			normals = computeNormals(heights);
			/*
			normals(heights.cols, heights.rows);
			vector<vec3> neighboors;
			neighboors.reserve(4);
			//loop through how Array2d is actually located in memory
			#define VERT(x,y,c) vec3((float)(x)/(heights.cols-1), (c)?heights[x][y]:0, (float)(y)/(heights.rows-1))
			for (size_t x=0; x < heights.cols; x++){
				for (size_t y=0; y < heights.rows; y++){
					vec3 me = VERT(x,y,true)
						,n(0);
					neighboors.clear();
					neighboors.push_back(VERT((int)x-1,y,x!=0)-me);
					neighboors.push_back(VERT(x,(int)y-1,y!=0)-me);
					neighboors.push_back(VERT(x+1,y,x != heights.cols-1)-me);
					neighboors.push_back(VERT(x,y+1,y != heights.rows-1)-me);

					for (size_t i=0; i < neighboors.size()-1; i++){
						n += normalize(cross(neighboors[i+1], neighboors[i]));//not sure if needs normalized here
					}

					if (NaN(n.x)||NaN(n.y)||NaN(n.z))
						cout <<"NaN normal"<<endl;
					normals[x][y] = normalize(n);
				}
			}
			#undef VERT
			*/
			msg("saving normals to",normalsFile);
			normals.write(normalsFile.c_str());
		}
		normalMap("normalMap",this, new Texture(normals.value_ptr(),normals.cols,normals.rows,GL_RGB32F,GL_RGB,GL_FLOAT,false,GL_CLAMP_TO_EDGE));

		auto vao = new VAO(GL_PATCHES);
		vao->bind(indices.size());
		vao->buffer(vertices.value_ptr(),vertices.bytes());
		vao->in(2,GL_FLOAT);
		vao->buffer(&indices[0],indices.size()*sizeof(I));
		vao->indices(ITYPE);
		vao->unbind();
		setVAO(vao);

		//Shadow uniforms
		enableShadowCast("tess.vert","shadowTess.frag","tess.geom","tess.tcs.glsl","tess.tes.glsl");
		shadowHeightMap("heightMap",&shadow,heightMap.value);
		shadowLod("lod",&shadow,lod.value);
	}

	float getY(float x, float z){
		vec4 local = getInverseTransform() * vec4(x,0,z,1);
		x = local.x * heights.cols;
		z = local.z * heights.rows;
		if (x < 0 || x > heights.cols-2 || z < 0 || z > heights.rows - 2) return -1;//not in bounds

		float y = biLinearInterpolate<float>(heights[(int)x][(int)z], heights[(int)x+1][(int)z], heights[(int)x][(int)z+1], heights[(int)x+1][(int)z+1], fract(x), fract(z));
		
		return (getWorldTransform() * vec4(x,y,z,1)).y;
	}

	vec3 getNormal(float x, float z){
		vec4 local = getInverseTransform() * vec4(x,0,z,1);
		x = local.x * normals.cols;
		z = local.z * normals.rows;
		if (x < 0 || x > normals.cols-2 || z < 0 || z > normals.rows - 2) return vec3(0);//not in bounds

		vec3 normal = biLinearInterpolate<vec3>(normals[(int)x][(int)z], normals[(int)x+1][(int)z], normals[(int)x][(int)z+1], normals[(int)x+1][(int)z+1], fract(x), fract(z));
		
		return normalize(vec3(getNormalTransform() * vec4(normal,0)));
	}

	void setLod(float v){
		Shader::enable();
		lod = clamp(v,.00001f, 1.f);
		shadow.enable();
		shadowLod = lod.value;
	}

	void setWaveAmp(float v){
		Shader::enable();
		waveAmp = v;
	}
};