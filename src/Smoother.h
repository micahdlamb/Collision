#pragma once

template <class I>
struct Smoother {
	Smoother():feedbackAvailable(false){}
	typedef glm::detail::tvec2<I> I2;
	//typedef glm::detail::tvec2<U> S2;
	
	typedef vec3 Vertex;

	struct OrientedEdge {
		U index;
		bool orientation;
		
		//computed
		I2 vertices;
	};

	struct OrientedFace {
		OrientedFace():index(-1){}
		U index;
		U edgeInFace;
	};

	struct F2 {OrientedFace x; OrientedFace y;};

	struct Edge {
		Edge():center(-1){}

		I2 vertices;
		F2 faces;//x = left face, y = right face

		//computed by sync
		Vertex midPt;

		//used by smoothers
		//catmull, loop
		I center;
		//doo sabin

		U connectedTo(U face){
			//assert("problem with face construction or invalid parameter" && faces.size() && !(faces.size() == 1 && faces[0] != face) && !(faces.size() == 2 && faces[0] != face && faces[1] != face) && faces.size() < 3);
			assert("no faces" && faces.x.index != -1);
			return faces.x.index != face ? faces.x.index : faces.y.index;
		}

		pair<U,U> lrfaces(I vertex){
			return vertices.x == vertex ? pair<U,U>(faces.x.index, faces.y.index) : pair<U,U>(faces.y.index, faces.x.index);
		}

		I otherVertex(U vertex){
			assert(vertex == vertices.x || vertex == vertices.y);
			return vertex == vertices.x ? vertices.y : vertices.x;
		}
	};

	struct Face {
		Face():center(-1){}
		vector<OrientedEdge> edges;
		
		//computed by sync
		vector<I> vertices;

		//used by smoothers
		//catmull
		I center;
		//doo sabin
		vector<I> facePts;
	};

	struct edge_cmp {
		bool operator()(I2 e1, I2 e2){
			I min1 = std::min(e1.x,e1.y)
				,min2 = std::min(e2.x,e2.y)
				,max1 = std::max(e1.x,e1.y)
				,max2 = std::max(e2.x,e2.y);

			return min1 < min2 || min1 == min2 && max1 < max2;
		}
	};

	vector<Vertex> vertices;
	vector<vector<U> > vertexEdges;//edges connected to each vertex

	vector<Edge> edges;
	typedef map<I2,U,edge_cmp> EdgeMap;
	EdgeMap edgeMap;//speed up build time
	vector<Face> faces;
	bool feedbackAvailable;

	void addEdge(U face, I2 vertices){
		
		EdgeMap::iterator i = edgeMap.find(vertices);

		if (i == edgeMap.end()){
			U index = edges.size();
			vertexEdges[vertices.x].push_back(index);
			vertexEdges[vertices.y].push_back(index);
			OrientedFace of;
			of.index = face;
			of.edgeInFace = faces[face].edges.size();
			Edge e;
			e.vertices = vertices;
			e.faces.x = of;
			OrientedEdge oe = {index,true};
			sync(e);
			edges.push_back(e);
			sync(oe);
			faces[face].edges.push_back(oe);
			edgeMap[vertices] = index;
		} else {
			U index = i->second;
			OrientedFace of;
			of.index = face;
			of.edgeInFace = faces[face].edges.size();
			edges[index].faces.y = of;
			OrientedEdge oe = {index,false};//i->first.x == vertices.x
			sync(oe);
			faces[face].edges.push_back(oe);
		}
	}

	void clear(){
		clearInput();
		clearOutput();
	}

	void clearInput(){
		vertices.clear();
		vertexEdges.clear();
		edges.clear();
		edgeMap.clear();
		faces.clear();
	}

	void clearOutput(){
		newVertices.clear();
		newIndices.clear();
		newFaces.clear();
		errors.clear();
		feedbackAvailable = false;
	}

	void setVertices(vector<Vertex>& v){
		vertices = v;
		vertexEdges.resize(v.size());
	}

	void addFace(I* indices, int size, I offset=0){
		U face = faces.size();
		faces.push_back(Face());
		for (int i=0; i < size; i++)
			addEdge(face, I2(indices[i]+offset, indices[((i+1)%size)]+offset));
		sync(faces[face]);
	}

	void operator()(vector<Vertex>& vertices, vector<I>& indices, int faceSize, bool makeTriangles=false){
		clear();
		setVertices(vertices);
		if (makeTriangles){
			I tri[3];
			for (U i=0; i  < indices.size(); i+=faceSize)
				for (int j=1; j < faceSize-1; j++){
					tri[0] = indices[i];
					tri[1] = indices[i+j];
					tri[2] = indices[i+j+1];
					addFace(tri, 3);
				}
		}	
		else
			for (U i=0; i  < indices.size(); i+=faceSize)
				addFace(&indices[i], faceSize);
	}

	void add(vector<Vertex>& vertices, vector<I>& indices, int faceSize){
		I offset = vertices.size();
		concat(this->vertices,vertices);
		vertexEdges.resize(vertices.size());
		for (U i=0; i  < vertices.size(); i+=faceSize)
			addFace(&indices[i], faceSize, offset);
	}

	//call these after making change to object
	void sync(Face& face){
		face.vertices.clear();
		for (auto i=face.edges.begin(); i != face.edges.end(); ++i)
			face.vertices.push_back(i->vertices.x);
	}

	void sync(Edge& edge){
		edge.midPt = (vertices[edge.vertices.x] + vertices[edge.vertices.y]) / 2.f;
	}

	void sync(OrientedEdge& oe){
		oe.vertices = oe.orientation ? edges[oe.index].vertices : edges[oe.index].vertices.swizzle(Y,X);
	}

	/////////////////
	//SMOOTHING STUFF
	/////////////////

	//the following is used for smoothing
	GLenum mode;
	vector<Vertex> newVertices;
	vector<I> newIndices;
	map<string, int> errors;

	I newVertex(Vertex& v){
		newVertices.push_back(v);
		return newVertices.size()-1;
	}

	void logError(string e){
		++errors[e];
	}

	void printErrors(){
		if (!errors.size()) return;
		cout << "errors: " << endl;
		for (auto i=errors.begin(); i != errors.end(); i++)
			cout << "\t" << i->first << " (" << i->second << " occurrences)\n";
	}

	Vertex calcCenter(Face& face){
		Vertex c(0);
		for (auto i=face.vertices.begin(); i != face.vertices.end(); ++i)
			c += vertices[*i];
		c /= face.vertices.size();
		return c;
	}

	vector<U> sortEdgesAroundVertex(U vertex, vector<U> indices){
		vector<U> r;
		r.push_back(indices.front());
		for (auto i = ++indices.begin(); i != indices.end(); i++){
			auto current = edges[r.back()].lrfaces(vertex);
			for (auto j = ++indices.begin(); j != indices.end(); j++){
				auto next = edges[*j].lrfaces(vertex);
				if (current.first == next.second){
					r.push_back(*j);
					break;
				}
			}
		}
		return r;
	}

	Vertex center(Face& face){
		if (face.center == (I)-1)
			return newVertices[centerIndex(face)];
		else
			return newVertices[face.center];
	}

	I centerIndex(Face& face){
		
		if (face.center != (I)-1)
			return face.center;

		Vertex c = calcCenter(face);

		//assert(face.vertices.size() == 4);

		face.center = newVertex(c);
		return face.center;
	}

	//store faces for feed back but output all triangles to newIndices
	vector<vector<I> > newFaces;
	void triangulateFace(vector<I>& f){
		if (f.size() < 3) return;//not sure if this should be allowed but its happening on edges
		newFaces.push_back(f);
		for (U i=1; i < f.size()-1; i++){
			newIndices.push_back(f[0]);
			newIndices.push_back(f[i]);
			newIndices.push_back(f[i+1]);
		}
	}

	bool feedback(){
		if (!feedbackAvailable) return false;
		clearInput();
		setVertices(newVertices);
		for (auto i=newFaces.begin(); i != newFaces.end(); ++i)
			addFace(&((*i)[0]), i->size());

		clearOutput();
		return true;
	}


	//////////////////
	//catmull helpers
	/////////////////

	Vertex center_cmc(Edge& edge){
		if (edge.center == (I)-1)
			return newVertices[centerIndex_cmc(edge)];
		else
			return newVertices[edge.center];

	}

	I centerIndex_cmc(Edge& edge){
		if (edge.center != (I)-1)
			return edge.center;

		Vertex c(0);

		int n = 3;
		c += center(faces[edge.faces.x.index]);
		if (edge.faces.y.index != -1){
			++n;
			c += center(faces[edge.faces.y.index]);
		}

		c += vertices[edge.vertices.x] + vertices[edge.vertices.y];
		//assert(n==4);
		c /= n;
		edge.center = newVertex(c);
		return edge.center;
	}



	////////////////////
	//doo sabin helpers
	////////////////////



	////////////////////
	//loop helpers
	////////////////////
	Vertex center_loop(Edge& edge){
		if (edge.center == (I)-1)
			return newVertices[centerIndex_loop(edge)];
		else
			return newVertices[edge.center];

	}

	I centerIndex_loop(Edge& edge){
		if (edge.center != (I)-1)
			return edge.center;

		Vertex c(0);

		if (edge.faces.y.index == -1)//edge case, midPt seems to work
			c = edge.midPt;
		else {
			c += (3.f/8) * vertices[edge.vertices.x];
			c += (3.f/8) * vertices[edge.vertices.y];
			auto& face1 = faces[edge.faces.x.index];
			auto& face2 = faces[edge.faces.y.index];
			c += (1.f/8) * vertices[face1.vertices[(edge.faces.x.edgeInFace+2)%face1.vertices.size()]];
			c += (1.f/8) * vertices[face2.vertices[(edge.faces.y.edgeInFace+2)%face2.vertices.size()]];
		}

		edge.center = newVertex(c);
		return edge.center;
	}

	/////////////////////
	//ALGS
	/////////////////////
	void catmull_clark(vector<Vertex>& vertices, vector<I>& indices, int faceSize){
		operator()(vertices, indices, faceSize);

		mode = GL_QUADS;
		feedbackAvailable = false;//doesn't use feedback

		for (I v=0; v < (I)vertices.size(); ++v){
			if (!vertexEdges[v].size()){
				logError("vertex with no edges, dropping faces");
				continue;
			}
			
			auto& vEdges = sortEdgesAroundVertex(v, vertexEdges[v]);
			Vertex avgEdgeMidPts(0);
			set<U> vertexFaces;
			for (U e=0; e < vEdges.size(); e++){
				auto& edge = edges[vEdges[e]];
				if (edge.faces.x.index != -1) vertexFaces.insert(edge.faces.x.index);
				if (edge.faces.y.index != -1) vertexFaces.insert(edge.faces.y.index);
				avgEdgeMidPts += edge.midPt;//center(edge);//
			}
			avgEdgeMidPts /= vEdges.size();
			
			Vertex facesAvg(0);
			for (auto i=vertexFaces.begin(); i != vertexFaces.end(); ++i)
				facesAvg += center(faces[*i]);
			facesAvg /= vertexFaces.size();

			float n = (float)vEdges.size();
			//Vertex vertex = (1/n)*facesAvg + (2/n)*avgEdgeMidPts + ((n-3)/3)*vertices[v];
			Vertex vertex = (facesAvg + 2.f*avgEdgeMidPts + (n-3)*vertices[v]) / n;
			I index = newVertex(vertex);


			//phew all points should be calculated, create indices
			for (U e=0; e < vEdges.size(); e++){
				//draw quad to left of edge
				auto& edge = edges[vEdges[e]];
				U leftFace = v == edge.vertices.x ? edge.faces.x.index : edge.faces.y.index;
				if (leftFace != -1){
					//find other edge, probably better ways to do this
					/*
					U bottomEdge=-1;
					for (U j=0; j < vEdges.size(); j++){
						if (j == e) continue;
						auto& other = edges[vEdges[j]];
						U rightFace = v == other.vertices.x ? other.faces.y.index : other.faces.x.index;
						if (leftFace == rightFace){
							bottomEdge = vEdges[j];
							break;
						}
					}
					
					//assert("2nd left face edge not found" && bottomEdge != -1);
					if (bottomEdge == -1)
						continue;
					*/

					U bottomEdge = vEdges[(e+1)%vEdges.size()];

					auto& lFace = faces[leftFace];
					auto& botEdge = edges[bottomEdge];
					//assert(botEdge.faces.x.index == leftFace || botEdge.faces.y.index == leftFace);
					if (!(botEdge.faces.x.index == leftFace || botEdge.faces.y.index == leftFace)){
						logError("disconnected edges around a vertice, dropping face");
						continue;
					}

					newIndices.push_back(index);
					newIndices.push_back(centerIndex_cmc(edge));
					newIndices.push_back(centerIndex(lFace));
					newIndices.push_back(centerIndex_cmc(botEdge));
				}
			}
		}
	}


	void doo_sabin(vector<Vertex>& vertices, vector<I>& indices, int faceSize){
		if (!feedback())
			operator()(vertices, indices, faceSize);

		mode = GL_TRIANGLES;
		
		for (U f=0; f < faces.size(); f++){
			auto& face = faces[f];
			Vertex faceCenter = calcCenter(face);
			for (U v = 0; v < face.vertices.size(); v++){
				Vertex facePt = vertices[face.vertices[v]] + faceCenter;
				U edge1 = v==0 ? face.edges.back().index : face.edges[v-1].index
					,edge2 = face.edges[v].index;
				facePt += edges[edge1].midPt;
				facePt += edges[edge2].midPt;
				facePt /= 4;
				face.facePts.push_back(newVertex(facePt));
			}

			//create indices for trianges on center of face
			triangulateFace(face.facePts);
			/*
			auto& fp = face.facePts;
			for (U i=1; i < fp.size()-1; i++){
				newIndices.push_back(fp[i]);
				newIndices.push_back(fp[i+1]);
				newIndices.push_back(fp[0]);
			}
			*/
		}

		for (U e=0; e < edges.size(); e++){
			auto& edge = edges[e];
			if (edge.faces.y.index == -1) continue;//only connected to 1 face
			auto& lface = faces[edge.faces.x.index];
			auto& rface = faces[edge.faces.y.index];

			I bl = lface.facePts[edge.faces.x.edgeInFace]
				,tl = lface.facePts[(edge.faces.x.edgeInFace + 1) % lface.facePts.size()]
				,tr = rface.facePts[edge.faces.y.edgeInFace]
				,br = rface.facePts[(edge.faces.y.edgeInFace + 1) % rface.facePts.size()];

			vector<I> nface;
			nface.push_back(bl);
			nface.push_back(br);
			nface.push_back(tr);
			nface.push_back(tl);
			triangulateFace(nface);
			/*
			newIndices.push_back(bl);
			newIndices.push_back(br);
			newIndices.push_back(tl);
			newIndices.push_back(br);
			newIndices.push_back(tr);
			newIndices.push_back(tl);
			*/
		}

		for (U v=0; v < vertices.size(); v++){
			vector<I> nface;
			//sort edges so winding ends up correct
			auto& vEdges = sortEdgesAroundVertex(v, vertexEdges[v]);

			
			//vEdges = sortEdgesAroundVertex(v,vEdges);

			for (U e=0; e < vEdges.size(); e++){
				auto& edge = edges[vEdges[e]];
				auto& lof = v == edge.vertices.x ? edge.faces.x : edge.faces.y;
				if (lof.index != -1){
					auto& lface = faces[lof.index];
					nface.push_back(lface.facePts[lof.edgeInFace]);
				}
			}

			triangulateFace(nface);
			/*
			for (U i=1; i < nface.size()-1; i++){
				newIndices.push_back(nface[i]);
				newIndices.push_back(nface[i+1]);
				newIndices.push_back(nface[0]);
			}
			*/
		}
		feedbackAvailable = true;
	}

	//faces must be added as triangles
	void loop(vector<Vertex>& vertices, vector<I>& indices, int faceSize){
		operator()(vertices, indices, faceSize, true);

		mode = GL_TRIANGLES;
		feedbackAvailable = false;//just reinitialize every time
		
		for (U f=0; f < faces.size(); ++f){
			auto& face = faces[f];
			assert("must be all triangles" && face.edges.size() == 3);
			for (U e=0; e < face.edges.size(); ++e)
				newIndices.push_back(centerIndex_loop(edges[face.edges[e].index]));
		}
		
		for (U v=0; v < vertices.size(); ++v){
			auto& vEdges = sortEdgesAroundVertex(v, vertexEdges[v]);
			U n = vEdges.size();
			float B = n == 3 ? (3.f/16) : (3.f/(8*n));

			Vertex neighborSum(0);
			for (U e=0; e < vEdges.size(); ++e)
				neighborSum += vertices[edges[vEdges[e]].otherVertex(v)];

			Vertex vertex = (1-n*B) * vertices[v] + B * neighborSum;

			//vertex = vertices[v];
			I index = newVertex(vertex);
			for (U e=0; e < vEdges.size(); ++e){
				newIndices.push_back(index);
				newIndices.push_back(centerIndex_loop(edges[vEdges[e]]));
				newIndices.push_back(centerIndex_loop(edges[vEdges[(e+1)%vEdges.size()]]));
			}
		}
	}
};