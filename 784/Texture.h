#pragma once

struct Texture {
	int width, height;
	GLuint gid;
	GLenum target;

	//used to make copies
	GLenum internalFormat;

	Texture(GLenum target=GL_TEXTURE_2D):target(target){
		glGenTextures(1, &gid);
	}

	Texture(void* data, int width, int height, GLenum internalFormat=GL_RGBA, GLenum format=GL_RGBA, GLenum type=GL_UNSIGNED_BYTE, GLenum wrap = GL_REPEAT, bool mipmaps=false):target(GL_TEXTURE_2D){
		glGenTextures(1, &gid);
		operator()(data, width, height, internalFormat, format, type, wrap, mipmaps);
	}

	void operator()(void* data, int width, int height, GLenum internalFormat=GL_RGBA, GLenum format=GL_RGBA, GLenum type=GL_UNSIGNED_BYTE, GLenum wrap = GL_REPEAT, bool mipmaps=false){
		this->width = width;
		this->height = height;
		this->internalFormat = internalFormat;

		bind();
		glTexImage2D(target, 0, internalFormat, width, height, 0, format, type, data);
		sampleSettings(wrap, mipmaps);
		//unbind();
	}

	void sampleSettings(GLenum wrap, bool mipmaps=true){
		glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri (target, GL_TEXTURE_WRAP_T, wrap);
		if (target == GL_TEXTURE_3D) glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap);

		glTexParameteri (target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (mipmaps){
			glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glGenerateMipmap(target);
		}
		else
			glTexParameteri (target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	void bind(){
		static GLuint bound0 = 0;
		if (bound0 != gid){
			activateUnit(0);
			glBindTexture(target, gid);
			bound0 = gid;
		}
	}

	//void unbind(){//does nothing
		//glBindTexture(target,0);
	//}

	void generateMipmaps(){
		bind();
		glGenerateMipmap(target);
	}

	//must use instead of glActiveTexture everywhere
	static void activateUnit(GLuint unit){
		static GLuint active = -1;
		if (unit != active){
			glActiveTexture(GL_TEXTURE0 + unit);
			active = unit;
		}
	}

	void save(const char* file){
		size_t size = 3 * width * height;
		char* data = new char[size];
		bind();
		glGetTexImage(target, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		//unbind();
		if (data == NULL){
			msg("error saving","");
			return;
		}

		ILuint id;
		ilGenImages(1,&id);
		ilBindImage(id);
		ilTexImage(width,height,1,3,IL_RGB, IL_UNSIGNED_BYTE, data);
		ilEnable(IL_FILE_OVERWRITE);
		ilSaveImage(file);
	}

	//incomplete
	Texture copyFormat(){
		Texture t(target);
		t(NULL,width,height,internalFormat);
		return t;
	}
};

struct ILTexture : public Texture {

	ILTexture(const char* file, ILenum origin = IL_ORIGIN_LOWER_LEFT):Texture(GL_TEXTURE_2D){
		//generates, binds, loads, creates mipmaps, deletes local copy
		ilEnable(IL_ORIGIN_SET);
		ilOriginFunc(origin);

		ILuint id;
		ilGenImages(1, &id);
		ilBindImage(id);
		ilLoadImage(file);
		printDevILErrors();
		int imageWidth = ilGetInteger( IL_IMAGE_WIDTH )
			,imageHeight = ilGetInteger( IL_IMAGE_HEIGHT);

		width = ceilPow2(imageWidth);
		height = ceilPow2(imageHeight);
		width = height = std::min(4096,std::max(width, height));

		unsigned char* data = new unsigned char[4*width*height];
		iluScale(width, height, 1);
		ilCopyPixels(0, 0, 0, width, height, 1, GL_RGBA, IL_UNSIGNED_BYTE, data);
		operator()(data, width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE,GL_REPEAT,true);
		ilDeleteImages  (1, &id);
	}
};

struct Texture3D : public Texture {
	int depth;

	Texture3D():Texture(GL_TEXTURE_3D){}
#define PARAMS void* data, int width, int height, int depth, GLenum internalFormat=GL_RGBA, GLenum format=GL_RGBA, GLenum type=GL_UNSIGNED_BYTE, GLenum wrap=GL_CLAMP_TO_BORDER, bool mipmaps=false
	Texture3D(PARAMS):Texture(GL_TEXTURE_3D){
		operator()(data,width,height,depth,internalFormat,format,type,wrap,mipmaps);
	}

	void operator()(PARAMS){
		this->width = width;
		this->height = height;
		this->depth = depth;
		this->internalFormat = internalFormat;
		bind();
		glTexImage3D(target, 0, internalFormat, width, height, depth, 0, format, type, data);
		sampleSettings(wrap, mipmaps);
		//unbind();
	}
#undef PARAMS
};


struct Texture1D : public Texture {

	Texture1D():Texture(GL_TEXTURE_1D){}
#define PARAMS void* data, int width, GLenum internalFormat=GL_RGBA, GLenum format=GL_RGBA, GLenum type=GL_UNSIGNED_BYTE, GLenum wrap=GL_REPEAT, bool mipmaps=false
	Texture1D(PARAMS):Texture(GL_TEXTURE_3D){
		operator()(data,width,internalFormat,format,type,wrap,mipmaps);
	}

	void operator()(PARAMS){
		this->width = width;
		bind();
		glTexImage1D(target, 0, internalFormat, width, 0, format, type, data);
		sampleSettings(wrap, mipmaps);
		//unbind();
	}
#undef PARAMS
};