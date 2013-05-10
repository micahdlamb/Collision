
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

#include "frame.h"
#include "jpeg.h"
#include "yuv.h"

const double r2y[3][3]= {
  {0.299, 0.587, 0.114},
  {-0.1687, -0.3313, 0.5},
  {0.5, -0.4187, -0.0813} };
const double y2r[3][3]={
  {1,0,1.402},
  {1,-0.34414,-0.71414},
  {1,1.772,0}};

inline void rgb2yuv(int rgb[3], int yuv[3]) {
  for (int i=0;i<3;i++) {
    double sum=0;
    for (int j=0;j<3;j++) sum+=r2y[i][j]*(rgb[j]-128);
    yuv[i]=int(sum+0.5);
  }
}

inline void yuv2rgb(int yuv[3], int rgb[3]) {
  for (int i=0;i<3;i++) {
    double sum=0;
    for (int j=0;j<3;j++) sum+=y2r[i][j]*yuv[j];
    rgb[i]=int(sum+0.5);
    rgb[i]+=128;
    if (rgb[i]<0) rgb[i]=0; 
    if (rgb[i]>255) rgb[i]=255;
  }
}

int finishdecoding(Frame &frame, Picture &pic) {
  int a[3]={5,67,12};
  int b[3],b2[3];
  int c[3]={250,117,216};
  int d[3],d2[3];
  rgb2yuv(a,b); yuv2rgb(b,b2);
  rgb2yuv(c,d); yuv2rgb(d,d2);

  int i,j,k;

  //NOTE: I do not use idloc here; as the JFIF
  //Interchange Format does not require it.

  pic.setsize(frame.x, frame.y);
  if (frame.nc==1) {  //Grayscale
    for (i=0;i<frame.y;i++)
      for (j=0;j<frame.x;j++) {
	k=frame.getdata(0,i,j);
	k+=128;  // Level shifting
	if (k<0) k=0; if (k>255) k=255;
	pic.setpixel(j,i,k,k,k);
      }
  } else {  //Color
    for (i=0;i<frame.y;i++)
      for (j=0;j<frame.x;j++) {

	for (k=0;k<3;k++) {
	  a[k]=frame.getdata(k,i/(frame.vmax/frame.c[k].v),
			     j/(frame.hmax/frame.c[k].h));
	}
	
	yuv2rgb(a,b);

	pic.setpixel(j,i,b[0],b[1],b[2]);

      }
  }

  return 1;
}

void beginencoding(Frame &frame, Picture &pic) {
  int i,j,k;
  int mcuwidth=frame.hmax*8;
  int mcuheight=frame.vmax*8;
  int virwidth=((pic.getwidth()+mcuwidth-1)/mcuwidth)*mcuwidth;
  int virheight=((pic.getheight()+mcuheight-1)/mcuheight)*mcuheight;
  frame.extenddata(pic.getheight()+mcuheight);
  if (frame.nc==1) {  //Grayscale
    for (i=0;i<frame.y;i++)
      for (j=0;j<frame.x;j++) {
	k=pic.getpixel(j,i)&0xFF;
	k-=128;  // Level shifting
	frame.setdata(0,i,j,k);
      }
    for (i=0;i<virheight;i++)
      for (j=frame.x;j<virwidth;j++)
	frame.setdata(0,i,j,0);
    for (i=frame.y;i<virheight;i++)
      for (j=0;j<frame.x;j++)
	frame.setdata(0,i,j,0);
  } else {  //Color
    for (k=0;k<3;k++) {
      for (i=0;i<virheight;i++)
	for (j=0;j<virwidth;j++)
	  frame.setdata(k,i,j,0);
    }
    int a[3], b[3];
    for (i=0;i<frame.y;i++)
      for (j=0;j<frame.x;j++) {
	pic.getpixel(j,i,a[0],a[1],a[2]);
	rgb2yuv(a,b);
	//NOTE: I do not use idloc here; as the JFIF
	//Interchange Format does not require it.
	for (k=0;k<3;k++)     //TODO: Take average!
	  frame.setdata(k,i/(frame.vmax/frame.c[k].v),
			j/(frame.hmax/frame.c[k].h),b[k]);
      }
  }
}
