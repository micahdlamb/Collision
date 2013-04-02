struct Perlin3D : public Texture3D {
	Perlin3D(int res, float freq=1){
		float* data = new float[res*res*res];
		for (int i=0; i < res; i++)
			for (int j=0; j < res; j++)
				for (int k=0; k < res; k++)
					data[i*res*res + j*res + k] = perlin(vec3(i/(float)res,j/(float)res,k/(float)res)*freq);

		operator()(data,res,res,res,GL_R32F,GL_RED,GL_FLOAT,false,GL_REPEAT);
		delete [] data;
	}
};