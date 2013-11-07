
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

#include "scan.h"
#include "jpeg.h"
#include "frame.h"
#include "huffman.h"
#include "myio.h"
#include "quant.h"
#include "datastruct.h"

#include <math.h>

const int scanorder[8][8]={ 
  { 0, 1, 5, 6,14,15,27,28},
  { 2, 4, 7,13,16,26,29,42},
  { 3, 8,12,17,25,30,41,43},
  { 9,11,18,24,31,40,44,53},
  {10,19,23,32,39,45,52,54},
  {20,22,33,38,46,51,55,60},
  {21,34,37,47,50,56,59,61},
  {35,36,48,49,57,58,62,63} };  
int nbits0based[7000];
int *nbits=nbits0based+3500;

int restartinterval;
int eoifound;

int buffered;
int buf[100];
int curbit,curbyte;
int getnextbit() {
  curbit%=8;
  if (!curbit) {
    if (buffered) {
      curbyte=buf[0];
      buffered--;
      for (int i=0;i<buffered;i++)
	buf[i]=buf[i+1];
    } else {
      curbyte=readbyte();
      if (curbyte==0xFF) {
	int y=readbyte();
	if (y!=0) {  // We have a marker
	  cerr<<"Wow! Unexpected Marker!\n";
	} else {
	
	}
      }
    }
  }
  int bit=(curbyte & (1<<(7-curbit)))>>(7-curbit);
  curbit++;

  return bit;
}

const double one_sqrt2=.70710678118654752440;
inline double C(int x) {
  if (x==0) return one_sqrt2;
  else return 1.0;
}
const double sqrt1_8=.35355339059327376220;
inline double C2(int x) {  //coeffiecients from DAB
  if (x==0) return sqrt1_8; 
  else return 0.5; 
}
const float Carr[8]={one_sqrt2,1,1,1,1,1,1,1};
const float C2arr[8]={sqrt1_8,.5,.5,.5,.5,.5,.5,.5};

//Please note that Carr= 2*C2arr :)

typedef float matrix[8][8];
const float pi=3.14159265358979323846;
float cs[200];
struct FF {
  FF() {
    for (int i=0;i<200;i++)
      cs[i]=cos(i*pi/16);
    for (int i=-3000;i<=3000;i++) {
      int j,k;
      if (i<0) j=-i; else j=i;
      //      cerr<<j<<"   ";
      for (k=0;j;k++,j/=2);
      //      cerr<<k<<endl;
      nbits[i]=k;
    }
  }
} ffffd;

inline void transpose(float (*a)[8]) {
  int i,j;
  for (i=0;i<8;i++)
    for (j=i+1;j<8;j++) {
      float t=a[i][j]; a[i][j]=a[j][i]; a[j][i]=t;
    }
}

inline void idctonrow(float *a, float *b) {
  for (int i=0;i<8;i++) {
    b[i]=0;
    for (int k=0;k<8;k++)
      b[i]+=(C2arr[k])*a[k]*cs[k*(2*i+1)];
  }
}
inline void dctonrow(float *a, float *b) {
  for (int i=0;i<8;i++) {
    b[i]=0;
    for (int k=0;k<8;k++)
      b[i]+=a[k]*cs[i*(2*k+1)]; 
    b[i]*=C2arr[i];
  }
}

inline void idct2(float (*a)[8], float (*b)[8]) {
  float c[8][8];
  int i;
  for (i=0;i<8;i++)
    idctonrow(a[i],c[i]);
  transpose(c);
  for (i=0;i<8;i++)
    idctonrow(c[i],b[i]);
  transpose(b);
}
inline void dct2(float (*a)[8], float (*b)[8]) {
  float c[8][8];
  int i;
  for (i=0;i<8;i++)
    dctonrow(a[i],c[i]);
  transpose(c);
  for (i=0;i<8;i++)
    dctonrow(c[i],b[i]);
  transpose(b);
}

inline void idct(int (*a)[8], int (*b)[8]) {
  int i,j;
  matrix c,d;
  for (i=0;i<8;i++)
    for (j=0;j<8;j++)
      c[i][j]=a[i][j];
  idct2(c,d);
  for (i=0;i<8;i++)
    for (j=0;j<8;j++)
      b[i][j]=int(floor(d[i][j]+0.5));
}
inline void dct(int (*a)[8], int (*b)[8]) {
  //  return;
  int i,j;
  matrix c,d;
  for (i=0;i<8;i++)
    for (j=0;j<8;j++)
      c[i][j]=a[i][j];
  dct2(c,d);
  for (i=0;i<8;i++)
    for (j=0;j<8;j++) {
      b[i][j]=int(d[i][j]+0.5);
      if (b[i][j]<-1023) b[i][j]=-1023;
      if (b[i][j]>1023) b[i][j]=1023;
    }
}

int nrestart;

int readscan(Frame &frame) {

  //cerr<<"Scanning...\n"<<flush;

  int Ls, // Length of scan header
    Ns;  // Number of components in scan
  int Cs[256],  //Component id
    DCid[256],  //id of DC Huffman table
    ACid[256];  //id of AC Huffman table

  Ls=read2bytes();
  Ns=readbyte();
  
  for (int i=0;i<Ns;i++) {
    Cs[i]=readbyte();
    int x=readbyte();
    DCid[i]=x>>4; ACid[i]=x & 0xF;
  }

  readbyte(); readbyte(); readbyte();  //SS, Se, and Ah:Al

  //Now, start scanning
  int MCUwidth, MCUheight;  //Width and height of MCU in final image
  int i,j,k,l,m,p,q;
  if (Ns==1) {
    MCUwidth=MCUheight=8;
  } else {
    MCUwidth=frame.hmax*8; MCUheight=frame.vmax*8;
  }

  int H[256],V[256];
  int htop[256], vtop[256];

  for (k=0;k<Ns;k++) {
    H[k]=frame.c[frame.idloc[Cs[k]]].h;
    V[k]=frame.c[frame.idloc[Cs[k]]].v;
  }

  if (Ns==1) htop[0]=vtop[0]=1;
  else {
    for (k=0;k<Ns;k++) {
      htop[k]=H[k];
      vtop[k]=V[k];
    }
  }
  
  int unit[8][8];
  int unit2[8][8];
  int zz[64];
  int prevDC[256];
  int x,y;
  int row, col;
  int runlen,ssss;

  curbit=0; curbyte=0;

  for (i=0;i<Ns;i++) prevDC[i]=0;

  for (i=0;frame.y==0 || i<(frame.y+MCUheight-1)/MCUheight;i++) {
    frame.extenddata((i+1)*MCUheight);
    for (j=0;j<(frame.x+MCUwidth-1)/MCUwidth;j++) {
      for (k=0;k<Ns;k++) { //Beginning of MCU
	hnode *DCroot=htable[DCTable][DCid[k]].root;
	hnode *ACroot=htable[ACTable][ACid[k]].root;
	for (l=0;l<vtop[k];l++) {
	  for (m=0;m<htop[k];m++) {
	    //read DC info
	    hnode *e=DCroot;
	    do {
	      e=e->child[getnextbit()];
	    } while (e->value<0);
	    x=0;
	    for (p=0;p<e->value;p++)
	      x |= (getnextbit() << (e->value-1-p));
	    if (!(x & (1<< (e->value-1)))) {
	      x|=(-(1<<(e->value))); x++;
	    }
	    zz[0]=prevDC[k]+x;
	    prevDC[k]=zz[0];
	    hnode *f;
	    q=1;
	    for(;;) {
	      f=ACroot;
	      do {
		f=f->child[getnextbit()];
	      } while (f->value<0);
	      if (f->value==0) {   //EOR - End of Run
		for (p=q;p<64;p++) zz[p]=0;
		break;
	      }
	      runlen=(f->value)>>4; ssss=(f->value)&0xF;
	      for (p=0;p<runlen;p++)
		zz[q++]=0;
	      x=0;
	      for (p=0;p<ssss;p++)
		x |= (getnextbit() << (ssss-1-p));
	      if (!(x & (1<< (ssss-1)))) {
		x|=(-(1<<(ssss))); x++;
	      }
	      zz[q++]=x;
	      if (q==64) break;
	    }

	    for (row=0;row<8;row++) 
	      for (col=0;col<8;col++)
		unit[row][col]=zz[scanorder[row][col]]      //reading the values
		  * qtable[frame.c[frame.idloc[Cs[k]]].Tq][scanorder[row][col]]; //dequantization

	    idct(unit,unit2);

	    for (row=0;row<8;row++)
	      for (col=0;col<8;col++)
		frame.setdata(frame.idloc[Cs[k]],
			      8*(i*vtop[k]+l)+row,
			      8*(j*htop[k]+m)+col,
			      unit2[row][col]);
	    
	    
	    
	  }
	}
      }  //End of MCU

      //Check for markers

      //if there are 0-bits left in the byte, there can't be a marker
      for (p=0;p<8-curbit;p++) 
	if ( !((1<<p)&curbyte) ) goto nomarker;

      for(;;) {  //all remaining bits are 1's; next byte is either a marker or part of the coding...
	x=readbyte();
	if (x==0xFF) {
	  y=readbyte();
	  if (y==0) { //Not a marker
	    buf[buffered++]=x;
	    break;
	  } else {  //Marker
	    curbit=0;
	    while (y==0xFF) y=readbyte();
	    if (y>=0xD0 && y<=0xD7) { // RSTm - Restart Marker
	      for (int fff=0;fff<k;fff++) prevDC[fff]=0;
	    } else if (y==0xDC) {     // DNL  - Define Number of Lines
	      read2bytes();
	      frame.y=read2bytes();
	    } else if (y==0xDD) {     // DRI  - Define Restart Interval
	      read2bytes();
	      restartinterval=read2bytes();
	    } else if (y==0xD9) {     // EOI  - End of Image
	      eoifound=1; goto stop_scanning;
	    } else {
	      Error("Unknown marker found in scan!");
	      int L=read2bytes();
	      for (int jkl=2;jkl<L;jkl++) readbyte();
	    }
	  }
	} else { // Not a marker: undo it!
	  buf[buffered++]=x;
	  break;
	}
      }
    nomarker:;

      if (nError) return 0;
    }
  }
 stop_scanning:;
  
  if (nError) return 0;
  return 1;
}

LList <MCUdata> * calscan(Frame &frame) {
  //Please note that components are interlaced.
  //I have not yet implemented a non-interlaced version

  LList <MCUdata> *mculist=new LList<MCUdata>;
  MCUdata data; data.setsize(200);

  int Ns=frame.nc;
  int MCUwidth, MCUheight;  //Width and height of MCU in final image
  int i,j,k,l,m,p,q;

  int htop[256],vtop[256];
  if (Ns==1) {
    htop[0]=vtop[0]=1;
    MCUwidth=MCUheight=8;
  } else {
    for (k=0;k<Ns;k++) {
      htop[k]=frame.c[k].h;
      vtop[k]=frame.c[k].v;
    }
    MCUwidth=frame.hmax*8; MCUheight=frame.vmax*8;
  }
  int unit[8][8], unit2[8][8];
  int zz[65];

  int prevDC[256];
  for (i=0;i<Ns;i++) prevDC[i]=0;
  int huffid[256];
  for (i=0;i<Ns;i++)
    if (i==0) huffid[i]=0; 
    else huffid[i]=1;

  for (i=0;i<(frame.y+MCUheight-1)/MCUheight;i++) {
    for (j=0;j<(frame.x+MCUwidth-1)/MCUwidth;j++) {
      for (k=0;k<Ns;k++) {
	for (l=0;l<vtop[k];l++) {
	  for (m=0;m<htop[k];m++) {
	    for (p=0;p<8;p++)
	      for (q=0;q<8;q++)
		unit[p][q]=frame.getdata(k,8*(i*vtop[k]+l)+p,
					 8*(j*htop[k]+m)+q);
	    dct(unit,unit2);

	    int &Tq=frame.c[k].Tq;
	    for (p=0;p<8;p++)
	      for (q=0;q<8;q++) {
		int &Q=qtable[Tq][scanorder[p][q]];
		zz[scanorder[p][q]]=(unit2[p][q]+(Q/2))/Q;   //Quantization
	      }
	    data.reset();
	    int delta=zz[0]-prevDC[k];
	    prevDC[k]=zz[0];  //BUG: forgot this!
	    int ssss=nbits[delta],rrrr;
	    if (delta<0) delta--;
	    data.add(ssss); 
	    huffcnt[DCTable][huffid[k]][ssss]++;
	    if (ssss) data.add(delta);
	    zz[64]=1500;
	    for (p=0;;) {
	      for (q=1;!zz[p+q] && q<16;q++);
	      p+=q;
	      if (p==64) {   //EOR - End of Run
		huffcnt[ACTable][huffid[k]][0]++;
		data.add(0);
		break;
	      }
	      rrrr=q-1;ssss=nbits[zz[p]];
	      int x=(rrrr<<4)|ssss;
	      huffcnt[ACTable][huffid[k]][x]++;
	      data.add(x);
	      if (zz[p]) {
		if (zz[p]>0) data.add(zz[p]);
		else data.add(zz[p]-1);
	      }
	      if (p==63) break;
	    }
	    mculist->append(data);
	  }
	}
      }
    }
  }
  return mculist;
}

void writebits(int nbits, int bits) {
  while (nbits) {
    int k=8-curbit;
    if (k>nbits) k=nbits;
    nbits-=k;
    curbyte<<=k;  //BUG: I had forgotten this!
    curbyte |= (((((1<<k)-1)<<nbits)&bits)>>nbits);
    curbit+=k;
    if (curbit==8) {
      writebyte(curbyte); 
      if (curbyte==0xFF) writebyte(0);
      curbyte=0; curbit=0;
    }
  }
}

void writescan(LList <MCUdata> *mculist, Frame &frame) {

  write2bytes(0xFFDA);  //SOS - Start of Scan
  
  int Ns=frame.nc;
  int MCUwidth, MCUheight;  //Width and height of MCU in final image
  int i,j,k,l,m,p,q;

  write2bytes(6+2*Ns);
  writebyte(Ns);

  for (i=0;i<Ns;i++) {
    writebyte(i+1);         //Component id
    if (i==0) writebyte(0); //id of DC and AC tables
    else writebyte(0x11);   
  }
  writebyte(0);
  writebyte(63);
  writebyte(0);

  MCUdata *data;
  int htop[256],vtop[256];
  if (Ns==1) {
    htop[0]=vtop[0]=1;
    MCUwidth=MCUheight=8;
  } else {
    for (k=0;k<Ns;k++) {
      htop[k]=frame.c[k].h;
      vtop[k]=frame.c[k].v;
    }
    MCUwidth=frame.hmax*8; MCUheight=frame.vmax*8;
  }
  int unit[8][8], unit2[8][8];
  int huffid[256];
  for (i=0;i<Ns;i++)
    if (i==0) huffid[i]=0; 
    else huffid[i]=1;

  curbit=0; curbyte=0;

  LList <MCUdata> :: entry *e=mculist->first();
  for (i=0;i<(frame.y+MCUheight-1)/MCUheight;i++) {
    for (j=0;j<(frame.x+MCUwidth-1)/MCUwidth;j++) {
      for (k=0;k<Ns;k++) {
	for (l=0;l<vtop[k];l++) {
	  for (m=0;m<htop[k];m++) {
	    data=&(e->x);
	    int ssss,rrrr;
	    ssss=data->data[0]; p=1;
	    writebits(htable[DCTable][huffid[k]].slen[ssss],
		      htable[DCTable][huffid[k]].scode[ssss]);
	    if (ssss) {
	      p++;
	      int x=data->data[1];
	      writebits(ssss,x);
	    }
	    for (;p<data->k;) {
	      int x=data->data[p++];

	      writebits(htable[ACTable][huffid[k]].slen[x],
			htable[ACTable][huffid[k]].scode[x]);
	      ssss=x & 0xF;
	      if (ssss) {
		x=data->data[p++];
		writebits(ssss,x);
	      }
	    }
	    e=e->next;
	  }
	}
      }
    }
  }
  writebits(8-curbit,(1<<20)-1);  //Finish unfinished byte

}
