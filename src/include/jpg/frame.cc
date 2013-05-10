
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

#include <algorithm>

#include "frame.h"
#include "myio.h"

int Frame::read() {
  int Lf;  //Length of segment
  int hv;

  Lf=read2bytes();
  P=readbyte();
  y=read2bytes(); x=read2bytes();
  nc=readbyte();

  hmax=vmax=-1;
  for (int i=0;i<nc;i++) {
    c[i].id=readbyte();
    idloc[c[i].id]=i;
    hv=readbyte();
    c[i].h=hv>>4; c[i].v=hv & 0xF;
    if (c[i].h>hmax) hmax=c[i].h; if (c[i].v>vmax) vmax=c[i].v;
    c[i].Tq=readbyte();
  }

  if (nError) return 0;
  return 1;
}

void Frame::extenddata(int row) {
  for (;ndatarow<row;ndatarow++) {
    data[ndatarow]=new int * [nc];
    for (int i=0;i<nc;i++)
      data[ndatarow][i]=new int[x+40];  //TODO: this can be reduced easily
  }
}

void Frame::cleardata() {
  for (int i=0;i<ndatarow;i++) {
    for (int j=0;j<nc;j++)
      delete [] data[i][j];
    delete [] data[i];
  }
  ndatarow=0;
}

Frame::~Frame() {
  cleardata();
}

void Frame::write() {
  write2bytes(0xFFC0);  //SOF0 - Start of Frame 0: Baseline DCT
  write2bytes(8+3*nc);  //Length of marker segment
  writebyte(P);
  write2bytes(y); write2bytes(x);
  writebyte(nc);
  for (int i=0;i<nc;i++) {
    writebyte(c[i].id);
    writebyte((c[i].h<<4)|(c[i].v));
    writebyte(c[i].Tq);
  }
}
