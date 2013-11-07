
/*             _      | |        |  _      | |      |  | |
 *            / \     | |          / \     | |  /|  |  | |             \
 *      Q___|____\  __| | | . |__Q____\  __| | (_|__|__| |  Q_|__|__|___)
 *  ___/    :      /      |___|         /               ___/          .
 *               _/                   _/
 *
 */

//  Written by Hamed Ahmadi Nejad
//    ahmadinejad@ce.sharif.edu
//    comphamed@yahoo.com

#ifndef frame_h
#define frame_h

#include <fstream>
#include <iostream>
using namespace std;

class Frame {
 private:
  int **data[63356]; //Never more than 65536 rows
  int ndatarow;   //Number of rows of data(above) which has been allocated

 public:
  int P;  //sample precision (in bits)
  int x, y;
  int nc; //Number of image components in frame

  struct Component {
    int id,  //Component id
      h, v, 
      Tq;    //Quantization table selector
    int x,y; //Width and height
  } c[256];
  int idloc[256];
  int hmax,vmax;

  Frame() {
    ndatarow=0;
  }
  ~Frame();
  void extenddata(int row);
  void setdata(int comp, int row, int col, int value) {
    data[row][comp][col]=value; 
  }
  int getdata(int comp, int row, int col) {
    return data[row][comp][col];  
  }
  void cleardata();
  int read();
  void write();
};

inline bool operator < (Frame::Component a,
			Frame::Component b) {
  return a.id<b.id;
}

#endif
