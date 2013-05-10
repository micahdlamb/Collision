
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

#ifndef huffman_h
#define huffman_h

#include <fstream>
#include <iostream>
using namespace std;

enum {DCTable, ACTable};

struct hnode {
  int value; //for leaves; -1 if not a leaf
  hnode *child[2];
  hnode() {
    value=-1;
    child[0]=child[1]=0;
  }
};

void dfskill(hnode *);

struct HTable {
  int L[17];      //info read from file
  int V[17][256];

  hnode *root;  //tree used in decoding

  int symbol[256];
  int len[257];
  int code[256];

  int slen[256]; //For encoding
  int scode[256]; //For encoding

  //int read(istream &f);  
  HTable() {
    root=0;
  }
  void setfromlength(int *a);

  void findcodes();
  void buildtree();
};

extern HTable htable[2][4];
extern int huffcnt[2][2][256];   //counters used in coding
//extern int ACcnt[2][256];

void dohuffman(int *a, int *b);

int readhtables();
void writehtables(int nh);

#endif
