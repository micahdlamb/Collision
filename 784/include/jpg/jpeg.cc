
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

#include <fstream>
#include <iostream>
using namespace std;

#include <cstring>

#include "jpeg.h"
#include "myio.h"
#include "quant.h"
#include "frame.h"
#include "huffman.h"
#include "scan.h"
#include "yuv.h"

Picture::Picture() {
  p=0;
}
Picture::Picture(int width, int height) {
  p=0;
  setsize(width,height);  
}
void Picture::setpixel(int x, int y, int r, int g, int b) {
  if (x<0 || x>=w || y<0 || y>=h) return;
  p[x][y].setcolor(r,g,b);
}
void Picture::setpixel(int x, int y, int rgb) {
  if (x<0 || x>=w || y<0 || y>=h) return;
  p[x][y].setcolor(rgb);
}
void Picture::getpixel(int x, int y, int &r, int &g, int &b) {
  if (x<0 || x>=w || y<0 || y>=h) {r=g=b=0; return; }
  p[x][y].getcolor(r,g,b);
}
int Picture::getpixel(int x, int y) {
  if (x<0 || x>=w || y<0 || y>=h) return 0;
  return p[x][y].getcolor();
}
void Picture::setsize(int width, int height) {
  int i,j;
  if (p) {
    for (i=0;i<w;i++) delete [] p[i];
    delete [] p;
  }
  w=width; h=height;
  p=new Pixel*[w];
  for (i=0;i<w;i++)
    p[i]=new Pixel[h];
  for (i=0;i<w;i++)
    for (j=0;j<h;j++)
      p[i][j].setcolor(0xffffff);
}
int Picture::open(char *filename) {
  ifstream f;
  int i,j,k,x,y,z;
  Frame frame;

  nError=0;
  restartinterval=0;
  eoifound=0;

  openfile(filename);
  if (nError) return 0;

  for (;;) { 
    
    for(;;) { //Find marker
      if (nError) return 0;
      x=readbyte();
      if (x == 0xFF) {
	y=readbyte();
	if (y!=0xFF && y!=0x00) break;
	f.unget();
      }
    }

    if (y==0xC0) {  //SOF - Start of Frame
      if (!frame.read()) return 0;
    } else if (y>=0xC1 && y<=0xCF && y!=0xC4) {
      cerr<<"Unsupported file format"<<endl;
      return 0;
    } else if (y==0xC4) {  //DHT - Define Huffman Table(s)
      if (!readhtables()) return 0;
    } else if (y>=0xD0 && y<=0xD7) {  //RSTm - Restart with modulo 8 count "m"
      //Restart m
    } else if (y==0xD8) {  //SOI - Start of Image
      restartinterval=0;
    } else if (y==0xD9) {  //EOI - End of Image
      eoifound=1;
      break;
    } else if (y==0xDA) {  //SOS - Start of Scan
      //Start scan
      if (!readscan(frame)) return 0;
      if (eoifound) break;
    } else if (y==0xDB) {  //DQT - Define Quantization Table(s)
      if (!readqtables()) return 0;
    } else if (y==0xDC) {  //DNL - Define Number of Lines
      //# of lines
    } else if (y==0xDD) {  //DRI - Define Restart Interval
      int Lr;
      Lr=read2bytes(); restartinterval=read2bytes();
    } else if (y==0xDE || y==0xDF) {
      cerr<<"Error - Marker for unsupported format found"<<endl;
      // try to pass it anyway
      int L=read2bytes(); for (int i=2;i<L;i++) readbyte();
    } else if ((y & 0xF0)==0xE0) {   //APPn - Application Extension (FFE0 - FFEF)
      int L=read2bytes(); for (int i=2;i<L;i++) readbyte();
    } else if (y>=0xF0 && y<=0xFD) { //JPGn - JPEG Extension
      int L=read2bytes(); for (int i=2;i<L;i++) readbyte();
    } else if (y==0xFE) {  //COM - Comment
      int L=read2bytes(); for (int i=2;i<L;i++) readbyte();
    } else if (y>=0x02 && y<=0xBF) { //RES - Reserved
      int L=read2bytes(); for (int i=2;i<L;i++) readbyte();
    }
  }
  if (nError) return 0;

  if (!finishdecoding(frame, *this)) 
    Error("Can't decode components into image");

  if (nError) return 0;
  return 1;
}

int Picture::save(char *filename, int quality, int h1, int v1) {
  //TODO: have parameter for restart markers
  nError=0;

  //cerr<<"Save initiated."<<endl<<flush;
  if (quality<1 || quality>100 || h1*v1>8 || 
      h1<1 || v1<1 || h1>4 || v1>4) {
    Error("Invalid parameters to save function!");
    return 0;
  }
  //Save to JPEG file
  resetwritebuffer();
  write2bytes(0xFFD8);  //SOI - Start of Image 

  write2bytes(0xFFE0);  //APP0 - For JFIF information
  write2bytes(16);      //Length of marker segment

  writestring("JFIF");
  writebyte(0);         //JFIF must be followed by a 0
  write2bytes(0x0102);  //version
  writebyte(0);
  write2bytes(1);       //x and y pixel aspect ratio
  write2bytes(1);
  write2bytes(0);       //No thumbnail

  write2bytes(0xFFFE);  //Comment
  char *comment1="   In The Name of God Almighty  ";
  write2bytes(2+strlen(comment1));  //2 bytes for length field
  writestring(comment1);

  write2bytes(0xFFFE);  //Comment
  char *comment=" Created by Hamed Ahmadi Nejad   email:comphamed@yahoo.com ";
  write2bytes(2+strlen(comment));  //2 bytes for length field
  writestring(comment);

  bool color=isColor();
  if (color) 
    writebothqtables(quality);
  else {
    writeluminanceqtable(quality);
    h1=v1=1;
  }

  for (int i=0;i<2;i++)
    for (int j=0;j<2;j++)
      for (int k=0;k<256;k++)
	huffcnt[i][j][k]=0;
  
  Frame frame;
  frame.P=8;  //8-bit precision
  frame.x=w; frame.y=h;
  if (color) frame.nc=3;
  else frame.nc=1;
  frame.c[0].id=1;
  frame.c[0].h=h1; frame.c[0].v=v1;
  frame.c[0].Tq=0;
  frame.idloc[1]=0;
  for (int i=1;i<frame.nc;i++) {
    frame.c[i].id=i+1;
    frame.c[i].h=1; frame.c[i].v=1;
    frame.c[i].Tq=1;
    frame.idloc[i+1]=i;
  }
  frame.hmax=frame.vmax=-1;
  for (int i=0;i<frame.nc;i++) {
    if (frame.c[i].h>frame.hmax) frame.hmax=frame.c[i].h;
    if (frame.c[i].v>frame.vmax) frame.vmax=frame.c[i].v;
  }

  beginencoding(frame,*this);
  LList <MCUdata> * mculist = calscan(frame);
  
  int len[256];
    
  dohuffman(huffcnt[0][0],len); htable[0][0].setfromlength(len);
  dohuffman(huffcnt[1][0],len); htable[1][0].setfromlength(len);
  if (color) {
    dohuffman(huffcnt[0][1],len); htable[0][1].setfromlength(len);
    dohuffman(huffcnt[1][1],len); htable[1][1].setfromlength(len);
  }

  writehtables(color?2:1);

  frame.write();
  writescan(mculist, frame);

  write2bytes(0xFFD9);  //EOI - End of Image

  write2bytes(0xFFFE);  //Comment
  write2bytes(2+strlen(comment));  //2 bytes for length field
  writestring(comment);

  writetofile(filename);  

  if (nError) return 0;
  return 1;
}

bool Picture::isColor() {
  for (int i=0;i<h;i++)
    for (int j=0;j<w;j++) {
      int r,g,b;
      getpixel(j,i,r,g,b);
      if (r!=g || r!=b || g!=b) return true;
    }
  return false;
}
