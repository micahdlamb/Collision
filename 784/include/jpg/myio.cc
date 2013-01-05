
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

#include "myio.h"
#include <fstream>
#include <iostream>
using namespace std;
#include <stdio.h>
#include <stdlib.h>

int nError;

const int chunksize=32768;
unsigned char *entire_file;
int filesize;
int fp;  //file pointer

void openfile(char *s) {
  FILE *f;
  f=fopen(s,"rb");
  if (!f) {
    Error("Unable to open specified file.");
    return;
  }
  fp=0;
  fseek(f,0,SEEK_END);
  filesize=ftell(f);
  rewind(f);  //for us, does the same as fseek(f,0,SEEK_SET);
  entire_file=new unsigned char[filesize];
  int d=filesize/chunksize,i;
  for (i=0;i<d;i++)
    fread(entire_file+i*chunksize,1,chunksize,f);
  fread(entire_file+i*chunksize,1,filesize%chunksize,f);
}

void closefile() {
  filesize=0; fp=0;
  delete [] entire_file;
}

int read2bytes() {
  if (fp+1>=filesize) {
    Error("Unexpected end of file");
    return -1;
  }
  int x=entire_file[fp]*0x100+entire_file[fp+1];
  fp+=2;
  return x;
}
int readbyte() {
  if (fp>=filesize) {
    Error("Unexpected end of file");
    return -1;    
  }
  return entire_file[fp++];
}

//  -------------------

#include "datastruct.h"

struct Chunk {
  unsigned char data[chunksize];
  int cnt;
  void add(int x) { data[cnt++]=x; }
  Chunk() {cnt=0;}
};

LList <Chunk *> buffer;

void resetwritebuffer() {
  buffer.clear();
  buffer.append(new Chunk);
}
void writebyte(int x) {
  if (buffer.last()->x->cnt==chunksize-1) 
    buffer.append(new Chunk);
  buffer.last()->x->add(x);
}
void write2bytes(int x) {
  if (buffer.last()->x->cnt>=chunksize-2) //Leave one byte hanging sometimes :)
    buffer.append(new Chunk);  
  Chunk *&bl=buffer.last()->x;
  bl->add(x>>8);
  bl->add(x & 0xFF);
}
void writestring(char *s) {
  for (;*s;s++) 
    writebyte(*s);
}
void writetofile(char *filename) {
  FILE *f;
  f=fopen(filename,"wb");
  if (!f) {
    Error("Unable to open specified file for writing");
    return;
  }
  LList <Chunk *> :: entry *e;
  for (e=buffer.first();e->next;e=e->next) 
    fwrite(e->x->data,1,e->x->cnt,f);
  fclose(f);  
}

//  -------------------

void Error(char *s) {
  cerr<<"Error : "<<s<<endl;
  nError++;
  //exit(1);
}

