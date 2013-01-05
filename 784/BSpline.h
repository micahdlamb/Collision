namespace BSpline {
	int detail = 10;
	template <class V>
	vector<V> quadratic_chaikin(vector<V> pts1, bool closed=false){
		vector<V> pts2;
		vector<V> *src = &pts2, *dst = &pts1;
		for (int i=0; i  < iterations; i++){
			swap(src,dst);
			if (closed)
				src->push_back((*src)[0]);
			dst->clear();
			for (size_t j=0; j < src->size()-1; j++){
				dst->push_back(.75f*(*src)[j] + .25f*(*src)[j+1]);
				dst->push_back(.25f*(*src)[j] + .75f*(*src)[j+1]);
			}
		}

		if (closed)
			dst->push_back((*dst)[0]);

		return *dst;
	}

	template <class V>
	vector<V> cubic(vector<V> pts, bool closed = false){
		if (closed && pts.size() > 3)
			pts.insert(pts.end(), pts.begin(), pts.begin()+3);
		
		vector<V> r;
		for (int i=0; i < (int)pts.size()-3; i++){
			for (int j=0; j <= detail; j++){
				float u = (float)j / (detail);
				float iu = 1.0f-u;

				float b0 = iu*iu*iu/6.0f
					,b1 = (3*u*u*u - 6*u*u + 4)/6.0f
					,b2 = (-3*u*u*u + 3*u*u + 3*u + 1)/6.0f
					,b3 = u*u*u/6.0f;
				r.push_back(b0*pts[i] + b1*pts[i+1] + b2*pts[i+2] + b3*pts[i+3]);
			}

			if (i != pts.size()-4)
				r.pop_back();//prevent duplicates
		}
		return r;
	}



	template <class V>
	struct Cell {
		V bEdge, lEdge, blCorner, center;
	};

	template <class V>
	vector<vector<V> > cubic_surface(vector<vector<V> >& grid, int detail, bool wrapX, bool wrapY){
		size_t cx = wrapX ? 0 : 1
			,cy = wrapY ? 0 : 1;
		Array2d<Cell<V>> cells(grid.size()-cx, grid[0].size()-cy);
		
		//calc face pts
		for (size_t x=0; x < cells.cols; x++)
			for (size_t y=0; y < cells.rows; y++){
				auto& c = cells[x][y];
				size_t xp1 = (x+1) % grid.size()
					,yp1 = (y+1) % grid[0].size();
				c.center = (grid[x][y] + grid[xp1][y] + grid[x][yp1] + grid[xp1][yp1]) / 4.f;
			}
		//calc edge pts & bottom left corner
		for (size_t x=0; x < cells.cols; x++)
			for (size_t y=0; y < cells.rows; y++){
				auto& c = cells[x][y];
				size_t xp1c = (x+1) % cells.cols
					,xm1c = x==0 ? cells.cols-1 : x-1//::mod((int)x-1, cells.cols)
					,yp1c = (y+1) % cells.rows
					,ym1c = y==0 ? cells.rows-1 : y-1//::mod((int)y-1, cells.rows)
					,xp1g = (x+1) % grid.size()
					,xm1g = x==0 ? grid.size()-1 : x-1//::mod((int)x-1, grid.size())
					,yp1g = (y+1) % grid[0].size()
					,ym1g = y==0 ? grid[0].size()-1 : y-1;//::mod((int)y-1, grid[0].size());
				
				//if (x != 0)
					c.lEdge = (grid[x][y] + grid[x][yp1g] + cells[x][y].center + cells[xm1c][y].center) / 4.f;
				//if (y != 0)
					c.bEdge = (grid[x][y] + grid[xp1g][y] + cells[x][y].center + cells[x][ym1c].center) / 4.f;
			
				//if (x != 0 && y != 0){
					V Q = (cells[x][y].center + cells[xm1c][y].center + cells[x][ym1c].center + cells[xm1c][ym1c].center) / 4.f;
					V R = (grid[x][y] + grid[xp1g][y]) / 2.f;
					R += (grid[x][y] + grid[xm1g][y]) / 2.f;
					R += (grid[x][y] + grid[x][yp1g]) / 2.f;
					R += (grid[x][y] + grid[x][ym1g]) / 2.f;
					R /= 4.f;
					auto& S = grid[x][y];
					c.blCorner = (Q + 2.f * R + S) / 4.f;
				//}
			}
		/*
		for (size_t y=0; y < cells.rows; y++){
			for (size_t x=0; x < cells.cols; x++)
				cout << cells[x][y].center.x << " " << cells[x][y].center.y << " " << cells[x][y].center.z << ", ";
			cout << endl;
		}
		*/

		//allocate memory
		vector<vector<V> > r(cells.cols*2-cx);
		for (size_t x=0; x < r.size(); x++)
			r[x].resize(cells.rows*2-cy);
		
		for (size_t x=cx; x < cells.cols; x++)
			for (size_t y=cy; y < cells.rows; y++){
				size_t _x = x*2-cx, _y = y*2-cy;
				size_t xp1r = (_x+1) % r.size()
					,xm1r = _x==0 ? r.size()-1 : _x-1
					,yp1r = (_y+1) % r[0].size()
					,ym1r = _y==0 ? r[0].size()-1 : _y-1
					,xp1c = (x+1) % cells.cols
					,xm1c = x==0 ? cells.cols-1 : x-1
					,yp1c = (y+1) % cells.rows
					,ym1c = y==0 ? cells.rows-1 : y-1;

				r[_x][_y] = cells[x][y].blCorner;
				r[xm1r][ym1r] = cells[xm1c][ym1c].center;
				r[_x][ym1r] = cells[x][ym1c].lEdge;
				r[xp1r][ym1r] = cells[x][ym1c].center;
				r[xp1r][_y] = cells[x][y].bEdge;
				r[xp1r][yp1r] = cells[x][y].center;
				r[_x][yp1r] = cells[x][y].lEdge;
				r[xm1r][yp1r] = cells[xm1c][y].center;
				r[xm1r][_y] = cells[xm1c][y].bEdge;
			}
		return r;
	}


	/*

	//cuz glm doesn't provide this
	template <class T>
	struct mat1x4 {
		typedef glm::detail::tvec4<T> V4;
		V4 v;
		mat1x4(T x, T y, T z, T w):v(x,y,z,w){}
		T operator*(const V4 v2) const {
			return dot(v, v2);
		}

		//cant figure out how to template matrices of different types
		mat1x4<T> operator *(const detail::tmat4x4<T> m) const {
			mat1x4<T> r;
			r.v.x = v.x * m[0][0] + v.y * m[0][1] + v.z * m[0][2] + v.w * m[0][3];
			r.v.y = v.x * m[1][0] + v.y * m[1][1] + v.z * m[1][2] + v.w * m[1][3];
			r.v.z = v.x * m[2][0] + v.y * m[2][1] + v.z * m[2][2] + v.w * m[2][3];
			r.v.w = v.x * m[3][0] + v.y * m[3][1] + v.z * m[3][2] + v.w * m[3][3];
			return r;
		}

		V4 transpose() const {
			return v;
		}
	};



	template <class V>
	detail::tmat4x4<V> toMatrix(vector<vector<V> >& grid, size_t x, size_t y){
		detail::tmat4x4<V> r;
		for (size_t i=x; i < x+4; i++)
			for (size_t j=y; j < y+4; j++)
				r[x][y] = grid[x][y];
		return r;
	}

	template <class V>
	vector<vector<V> > cubic_surface(vector<vector<V> >& grid, int detail){

		float split ={
			4,4,0,0,
			1,6,1,0,
			0,4,4,0,
			0,1,6,1
		};
		mat4 H = make_mat4(split);


		float m[] = {
			-1,3,-3,1,
			3,-6,3,0,
			-3,0,3,0,
			1,4,1,0
		};
		mat4 M = (1/6)*make_mat4(m);

		Array2d<V> surface(grid.size() * detail, grid[0].size() * detail);
		for (size_t x=0; x < grid.size()-4; x++){
			for (size_t y=0; y < grid[x].size()-4; y++){
				detail::tmat4x4<V> P = toMatrix(grid, x, y);
				for (int i=0; i < detail; i++)
					for (int j=0; j < detail; j++){
						float u = i/(float)detail;
						float w = j/(float)detail;
						mat1x4<float> U(u*u*u,u*u,u,1);
						mat1x4<float> W(w*w*w,w*w,w,1);
						//surface[detail*x + i][detail*y + j] = U * M * P * inverse(M) * W.transpose();
					}
			}
		}

		vector<vector<V> > r(surface.cols);
		for (size_t x = 0; x < r.size(); x++)
			r[x].assign(surface[x], surface[x+1]);

		return r;
	}

	*/
}