
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

#include "quant.h"
#include "myio.h"

const int scanorder[8][8]={ 
  { 0, 1, 5, 6,14,15,27,28},
  { 2, 4, 7,13,16,26,29,42},
  { 3, 8,12,17,25,30,41,43},
  { 9,11,18,24,31,40,44,53},
  {10,19,23,32,39,45,52,54},
  {20,22,33,38,46,51,55,60},
  {21,34,37,47,50,56,59,61},
  {35,36,48,49,57,58,62,63} };  

// The following tables are suggested in the ISO
// standard (note that they are only suggestions)
// It claims that if these are divided by 2, the
// resulting image is nearly indistinguishable 
// from the original.
const int Table[2][8][8] = { {
  //Luminance quantization table for 50% quality
  { 16, 11, 10, 16, 24, 40, 51, 61},
  { 12, 12, 14, 19, 26, 58, 60, 55},
  { 14, 13, 16, 24, 40, 57, 69, 56},
  { 14, 17, 22, 29, 51, 87, 80, 62},
  { 18, 22, 37, 56, 68,109,103, 77},
  { 24, 35, 55, 64, 81,104,113, 92},
  { 49, 64, 78, 87,103,121,120,101},
  { 72, 92, 95, 98,112,100,103, 99} },
			     		     
  //Chrominance quantization table for 50% quality
  {{17, 18, 24, 47, 99, 99, 99, 99},
  { 18, 21, 26, 66, 99, 99, 99, 99},
  { 24, 26, 56, 99, 99, 99, 99, 99},
  { 47, 66, 99, 99, 99, 99, 99, 99},
  { 99, 99, 99, 99, 99, 99, 99, 99},
  { 99, 99, 99, 99, 99, 99, 99, 99},
  { 99, 99, 99, 99, 99, 99, 99, 99},
  { 99, 99, 99, 99, 99, 99, 99, 99} } };

//int qtable[4][8][8];
int qtable[4][64];

int readqtables() {
  int Lq, //length of header
    Pq,  //precision (0 for 8-bit, 1 for 16-bit)
    Tq,  //destination (one of 4 locations)
    PT;
  int i,j,x;

  Lq=read2bytes();

  for (x=2;x<Lq;x+=(65+64*Pq)) {
    PT=readbyte();
    Pq=PT>>4; Tq=PT & 0xF;

    int (*readfunc)();  //pointer to function

    if (Pq==0) readfunc=readbyte;
    else if (Pq==1) readfunc=read2bytes;
    else { 
      Error("Invalid quantization table precision"); return 0;
    }
    if (Tq>3) {
      Error("Invalid quantization table destination"); return 0;
    }
      
    for (i=0;i<64;i++)
      qtable[Tq][i]=readfunc();

  }
  if (nError) return 0;
  return 1;
}

//id is 0 for Luminance, 1 for Chrominance
void writetablecore(int quality, int id) {
  writebyte(id);  //left nybble 0 for 8-bit precision, right nybble is table specifier
  for (int i=0;i<8;i++)
    for (int j=0;j<8;j++) {
      const int &a=Table[id][i][j];
      int x=(1-a)*quality/50+2*a-1;
      if (x<1) x=1; if (x>255) x=255;
      qtable[id][scanorder[i][j]]=x;
    }
  for (int i=0;i<64;i++)
    writebyte(qtable[id][i]);
}

void writeluminanceqtable(int quality) {   
  write2bytes(0xFFDB);  //DQT - Define Quantization Table(s)
  write2bytes(67);
  writetablecore(quality,0);
}

void writebothqtables(int quality) {
  write2bytes(0xFFDB);  //DQT - Define Quantization Table(s)
  write2bytes(132);
  writetablecore(quality,0);
  writetablecore(quality,1);
}
