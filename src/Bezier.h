namespace Bezier {

	template <class V>
	vector<V> oneSubdivide(vector<V> pts, vector<V> poly1, vector<V> poly2, float u){
		if (pts.size() == 1){
			poly1.push_back(pts[0]);
			concat(poly1, poly2);
			return poly1;
		}

		poly1.push_back(pts[0]);
		poly2.insert(poly2.begin(), pts.back());//push_front... innefficient
		for (size_t i=0; i < pts.size()-1; i++){
			pts[i] += u * (pts[i+1] - pts[i]);
		}
		pts.pop_back();
		return oneSubdivide<V>(pts, poly1, poly2, u);
	}

	template <class V>
	vector<V> subdivide(vector<V> pts, int m, float u){
		if (m == 1)
			return oneSubdivide<V>(pts, vector<V>(), vector<V>(), u);
		
		vector<V> p = oneSubdivide<V>(pts, vector<V>(), vector<V>(), u);
		vector<V>::const_iterator it1 = p.begin()
			,it2 = p.begin()+pts.size()
			,it3 = p.end();

		vector<V> r1 = subdivide<V>(vector<V>(it1,it2), m-1, u)
			,r2 = subdivide<V>(vector<V>(it2-1,it3), m-1, u);

		r1.pop_back();//remove duplicates

		concat(r1,r2);
		return r1;
	}

	template <class V>
	vector<V> create(vector<V> pts){
		return iterations > 0 ? subdivide<V>(pts, iterations, .5f) : pts;
	}

	template <class V>
	V casteljau(vector<V> pts, float u){
		if (pts.size() == 1)
			return pts[0];

		for (size_t i=0; i < pts.size()-1; i++){
			pts[i] += u * (pts[i+1] - pts[i]);
		}
		pts.pop_back();
		return casteljau(pts, u);
	}


	template <class V>
	vector<vector<V> > casteljauSurf(vector<vector<V> >& grid, int detail, bool wrapX, bool wrapY){
		if (wrapX){
			vector<V> m;
			for (size_t y=0; y < grid.front().size(); y++){
				V midPt = (grid.front()[y] + grid.back()[y]) / 2.f;
				m.push_back(midPt);
			}
			grid.insert(grid.begin(), m);
			grid.push_back(m);
		}

		if (wrapY){
			for (size_t x=0; x < grid.size();x++){
				V midPt = (grid[x].front() + grid[x].back()) / 2.f;
				grid[x].insert(grid[x].begin(), midPt);
				grid[x].push_back(midPt);
			}
		}
		
		float t = (float)detail;//don't gen u=1, closed curves will connect this
		vector<vector<V> > t1(grid.size());
		for (size_t x=0; x < grid.size(); x++){
			t1[x].resize(detail);
			for (int i=0; i < detail; i++)
				t1[x][i] = casteljau(grid[x], i/t);
		}

		vector<vector<V> > t2(detail);
		for (size_t y=0; y < t2.size(); y++){
			t2[y].resize(t1.size());
			for (size_t x=0; x < t1.size(); x++)
				t2[y][x] = t1[x][y];
		}

		vector<vector<V> > r(detail);
		for (int i=0; i < detail; i++){
			r[i].resize(t2.size());
			for (size_t y=0; y < t2.size(); y++)
				r[i][y] = casteljau(t2[y], i/t);
		}
		return r;

		/*
		for (size_t x=0; x < grid.size(); x++)
			grid[x] = subdivide(grid[x], detail, .5);

		vector<vector<V> > t(grid[0].size());
		for (size_t y=0; y < t.size(); y++){
			t[y].resize(grid.size());
			for (size_t x=0; x < grid.size(); x++)
				t[y][x] = grid[x][y];
		}

		for (size_t x=0; x < t.size(); x++)
			t[x] = subdivide(t[x], detail, .5);

		return t;
		*/

	}

	///////////////////////////////
	//unused
	///////////////////////////////
	/*


	void create2(){
		curvePts.resize(MAXCURVEPTS);
		vector<vec3> pts = getControlPtPositions();
		for (size_t i=0; i < MAXCURVEPTS; i++){
			float u = (float)i/MAXCURVEPTS;
			curvePts[i].pos = casteljau(pts, u);
			curvePts[i].color = curveColor;//meh never changes as of now
		}
		curvePts.pushAll();
	}
	*/

}