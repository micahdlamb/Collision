#pragma once

struct Grid2D {
	//stores all intersection information for the handler
	struct Result : public Intersect::Result, public pair<size_t,size_t> {
		//let me copy derived into base
		Result(Intersect::Result ir):Intersect::Result(ir){}
	};
	
	//intersect caller must pass one of these as an intersection handler
	struct IntersectionHandler {
		virtual void handleIntersection(Result r)=0;
	};

	typedef glm::detail::tvec2<size_t> Cell;

	#define X(a) (size_t)clamp((((a)-worldMin.x) / (worldMax.x-worldMin.x)) * cells.cols, 0.f, cells.cols-1.f)
	#define Z(a) (size_t)clamp((((a)-worldMin.y) / (worldMax.y-worldMin.y)) * cells.rows, 0.f, cells.rows-1.f)

	//returns min bound
	#define BOUNDX(a) (worldMin.x + ((float)(a) / cells.cols) * (worldMax.x - worldMin.x))
	#define BOUNDZ(a) (worldMin.y + ((float)(a) / cells.rows) * (worldMax.y - worldMin.y))

	vec2 worldMin, worldMax;
	//using indices to reference objects
	Array2d<vector<size_t>> cells;

	Grid2D(){}

	/*
	worldMin/worldMax: -x,-z and x,z corners of the world
	numCells: number of cells in the x & z dimensions.
	-cell dimensions -> (worldMax - worldMin)/numCells
	--each object must fit completely in a cell
	*/
	void operator()(vec2 worldMin, vec2 worldMax, uvec2 numCells){
		this->worldMin = worldMin;
		this->worldMax = worldMax;
		cells(numCells.x,numCells.y);
	}
	
	vector<IBoundingVolume*>* bvs;
	IntersectionHandler* ih;

	/*
	find intersections and call intersection handler
	bvs: array of bounding volume pointers
	ih: function to handle an intersection
	-receives a Result object containing indices of the 2 bounding volumes & hit pt
	*/
	int intersections;
	void intersect(vector<IBoundingVolume*>& bvs, IntersectionHandler* ih){
		clear();
		intersections = 0;
		this->bvs = &bvs;
		this->ih = ih;

		//add bvs to cells
		for (size_t i=0; i < bvs.size(); i++){
			auto bv = bvs[i];
			auto& pt = bv->center;
			cells[X(pt.x)][Z(pt.z)].push_back(i);
		}

		//could be race condition in handler but should be rare
		#pragma omp parallel for
		for (int x=0; x < cells.cols; x++)
			for (size_t y=0; y < cells.rows; y++){
				//intersect cell with itself
				intersect(cells[x][y]);
				
				bool left = x == 0
					,right = x == cells.cols-1
					,bottom = y == 0
					,top = y == cells.rows-1;
				//intersect neighboring cells
				if (!left)
					intersect(cells[x][y], cells[x-1][y+1]);
				if (!top)
					intersect(cells[x][y], cells[x][y+1]);
				if (!top && !right)
					intersect(cells[x][y], cells[x+1][y+1]);
				if (!right)
					intersect(cells[x][y], cells[x+1][y]);
			}
	}

	private:

	//intersect 2 dif cells
	void intersect(vector<size_t>& cell1, vector<size_t>& cell2){
		for (auto i=cell1.begin(); i != cell1.end(); i++)
			for (auto j=cell2.begin(); j != cell2.end(); j++){
				auto bv1 = (*bvs)[*i], bv2 = (*bvs)[*j];
				if (bv1->sleeping && bv2->sleeping) continue;
				Result r = bv1->intersect(bv2);
				if (r.intersect){
					r.first = *i;
					r.second = *j;
					intersections++;
					ih->handleIntersection(r);
				}
			}
	}

	//intersect cell with itself
	void intersect(vector<size_t>& cell){
		for (size_t i=0; i < cell.size(); i++)
			for (size_t j=i+1; j < cell.size(); j++){
				auto bv1 = (*bvs)[cell[i]], bv2 = (*bvs)[cell[j]];
				if (bv1->sleeping && bv2->sleeping) continue;
				Result r = bv1->intersect(bv2);
				if (r.intersect){
					r.first = cell[i];
					r.second = cell[j];
					intersections++;
					ih->handleIntersection(r);
				}
			}
	}

	void clear(){
		//hopefully clear doesn't actually deallocate
		#pragma omp parallel for
		for (int x=0; x < cells.cols; x++)
			for (size_t y=0; y < cells.rows; y++)
				cells[x][y].clear();
	}
};