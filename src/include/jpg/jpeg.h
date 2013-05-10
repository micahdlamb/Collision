
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

#ifndef jpeg_h
#define jpeg_h

#include "pixel.h"

class Picture {
   int w, h;
   Pixel **p;
   bool isColor();  //See if there is color in the picture

  public:
   Picture();
   Picture(int width, int height);
   int getwidth() {return w;}
   int getheight() {return h;}
   void setsize(int width, int height); //kills all image data
   void setpixel(int x, int y, int r, int g, int b);   
   void setpixel(int x, int y, int rgb);
   void getpixel(int x, int y, int &r, int &g, int &b);
   int getpixel(int x, int y);

   int open(char *filename); //Open a JPEG file
   int save(char *filename, int quality=75, int h1=2, int v1=2);  //Save to JPEG file
};

#endif
