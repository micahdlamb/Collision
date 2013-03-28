
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

#include "huffman.h"
#include "myio.h"
#include "datastruct.h"

HTable htable[2][4];

int huffcnt[2][2][256]; //counters used in coding

int readhtables() {
  int Lh,  //Length of marker segment
    Tc,   //Table class (0 for DC, 1 for AC)
    Th;   //Table destination
  int x,y;
  Lh=read2bytes();
  for (x=2;x<Lh;) {
    y=readbyte();
    Tc=y>>4; Th=y & 0xF;
    if (Tc!=0 && Tc!=1) {
      Error("Invalid Huffman table class"); return 0;
    }
    if (Th>3) {
      Error("Invalid Huffman table destination"); return 0;
    }
    for (int i=1;i<=16;i++)
      htable[Tc][Th].L[i]=readbyte();
    for (int i=1;i<=16;i++)
      for (int j=0;j<htable[Tc][Th].L[i];j++)
	htable[Tc][Th].V[i][j]=readbyte();
    htable[Tc][Th].findcodes();
    htable[Tc][Th].buildtree();

    x+=17;
    for (int i=0;i<=16;i++)
      x+=htable[Tc][Th].L[i];
  }

  if (nError) return 0;
  return 1;
}

void dfskill(hnode *x) {
  if (!x) return;
  dfskill(x->child[0]); dfskill(x->child[1]);
  delete x;
}

void HTable::findcodes() {
  int i,j,k;
  k=0;
  for (i=1;i<=16;i++) {
    for (j=0;j<L[i];j++) {
      symbol[k]=V[i][j];
      len[k]=i;
      k++;
    }
  }
  len[k]=0;

  int x=0,si=len[0],cd=0;
  for (i=0;;) {
    do {
      code[i]=cd;
      cd++; i++;
    } while (len[i]==si);
    if (len[i]==0) break;
    do {
      cd<<=1;
      si++;
    } while (len[i]!=si);
  }

  for (i=0;i<256;i++) slen[i]=0;
  for (i=0;i<k;i++) {  //to be used in encoding
    slen[symbol[i]]=len[i];
    scode[symbol[i]]=code[i];
  }

}

int p2[]={1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,
	  16384,32768,65536,131072,262144,524288,1048576};

void HTable::buildtree() {
  if (root) dfskill(root);
  root=new hnode;

  hnode *e;
  int i,j,x,b;
  for (i=0;i<256;i++) {
    for (e=root,j=0;j<slen[i];j++) {
      b=(p2[slen[i]-j-1] & scode[i])>>(slen[i]-j-1);
      if (!e->child[b]) e->child[b]=new hnode;
      e=e->child[b];
    }
    e->value=i;
  }
}

int globalcodelen[257];

struct huffnode {
  int x;
  huffnode *left, *right;
  huffnode *p, *bro;
  LList <huffnode *> :: entry *listentry;
};

struct heapnode {
  int value;
  huffnode *huffn;
};

inline bool operator < (heapnode a, heapnode b) {
  return a.value<b.value;
}

const int garbage=256;
LList <huffnode *> hlist[50];  //Though I honestly doubt that there would be codes of length 50

void dfs(huffnode *hf, int depth) {
  if (hf->left) {
    dfs(hf->left,depth+1);
    dfs(hf->right,depth+1);
  } else {
    //if (hf->x==garbage) return;    
    if (depth==0) depth++;
    globalcodelen[hf->x]=depth;
    hlist[depth].append(hf);
    hf->listentry=hlist[depth].last();
  }
}

void dohuffman(int *a, int *b) {
  int i,j,k;

  Heap <heapnode> heap(256);
  heap.n=0;
  for (i=0;i<256;i++) 
    if (a[i]) {
      heap.a[++heap.n].value=a[i];
      huffnode *e=new huffnode;
      e->left=e->right=0;
      e->x=i;
      heap.a[heap.n].huffn=e;
    }

  {  //Note: I put the braces here on purpose (to avoid possible name collisions)
    heap.a[++heap.n].value=0;
    huffnode *e=new huffnode;
    e->left=e->right=0;
    e->x=garbage;
    heap.a[heap.n].huffn=e;
  }

  heap.make();

  while (heap.n>1) {
    heapnode hpn=heap.extractmin();
    huffnode *e=new huffnode;
    e->left=hpn.huffn; e->right=heap.a[1].huffn;
    e->left->bro=e->right; e->right->bro=e->left;
    e->left->p=e->right->p=e;
    heap.a[1].value+=hpn.value;
    heap.a[1].huffn=e;
    heap.siftdown(1);
  }

  for (i=0;i<50;i++) hlist[i].clear();
  dfs(heap.a[1].huffn,0);

  for (i=45;i>16;i--) {
    while (hlist[i].nelem) {
      huffnode *e, *f, *g;

      e=hlist[i].first()->x; f=e->bro;
      if (f->x==garbage) {
	huffnode* t=f; f=e; e=t;  //Keep the sentinel node as low as possible (low means far from the root)
      }
      hlist[i].kill(e->listentry); hlist[i].kill(f->listentry);

      //Give e's data to its parent
      e->p->left=e->p->right=0;
      e->p->x=e->x;
      hlist[i-1].append(e->p);
      e->p->listentry=hlist[i-1].last();

      for (j=i-2; !hlist[j].nelem; j--); //find leaf higher in the tree
      g=hlist[j].first()->x; hlist[j].kill(hlist[j].first());

      //Give g's data to e; make g the parent of e and f
      e->x=g->x;
      g->left=e; g->right=f;
      e->p=f->p=g;
      hlist[j+1].append(e); e->listentry=hlist[j+1].last();
      hlist[j+1].append(f); f->listentry=hlist[j+1].last();
    }  //Note:  I found it beautiful that no brother pointers are changed!
  }
  dfs(heap.a[1].huffn,0);
  for (i=0;i<50;i++) hlist[i].clear();

  for (i=0;i<256;i++)
    if (a[i]) b[i]=globalcodelen[i];
    else b[i]=0;
 
}

void HTable::setfromlength(int *a) {
  for (int i=1;i<=16;i++) L[i]=0;
  for (int i=0;i<256;i++) {
    if (!a[i]) continue;
    //if (a[i]>16) a[i]=16;  //Will this work?  Answer:no
    V[a[i]][L[a[i]]++]=i;
  }
  findcodes();
}

void writehtables(int nh) {
  write2bytes(0xFFC4);  // DHT - Define Huffman Table(s)
  
  int len=2;
  int i,j,k,l;
  for (i=0;i<2;i++)
    for (j=0;j<nh;j++) {
      len+=17;
      for (k=16;k;k--) len+=htable[i][j].L[k];
    }
  write2bytes(len);
  for (i=0;i<2;i++) 
    for (j=0;j<nh;j++) {
      writebyte((i<<4)|j);
      for (k=1;k<=16;k++) writebyte(htable[i][j].L[k]);
      for (k=1;k<=16;k++)
	for (l=0;l<htable[i][j].L[k];l++)
	  writebyte(htable[i][j].V[k][l]);
    }      
}
